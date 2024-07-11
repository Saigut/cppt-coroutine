#include <mod_coroutine/mod_cor_impl.hpp>

#include <queue>
#include <map>
#include <memory>
#include <thread>
#include <condition_variable>
#include <boost/context/continuation.hpp>
#include <boost/asio/io_context.hpp>
#include <mod_atomic_queue/atomic_queue.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <mod_np_queue/mod_np_queue.hpp>
#include <mod_time_wheel/mod_time_wheel.hpp>
#include <boost/asio/steady_timer.hpp>
#include <mod_mempool/mod_mempool.hpp>

namespace cppt_impl {

    // Types
    namespace context = boost::context;
    using boost::asio::io_context;


    // Values
    static std::vector<np_queue_t<cppt_task_t>> g_task_queues(1);
    static std::vector<co_executor_sp> g_executors(1);
    static thread_local co_executor_sp g_executor;
    bool g_run_flag = true;
    static unsigned gs_core_num = 1;
    static unsigned gs_use_core = 0;
    static time_wheel_t gs_time_wheel(60);
    static std::shared_ptr<mempool> gs_tw_task_pool;

    static io_context g_io_ctx;


    // Helpers
    static void cppt_co_add_task_global(cppt_task_t&& task)
    {
        unsigned core_cnt = 0;
        do {
            if (g_task_queues[gs_use_core++ % gs_core_num]
                    .try_enqueue(std::move(task))) {
                return;
            }
            core_cnt++;
        } while (core_cnt <= gs_core_num);
        log_error("coroutine task queues full!");
    }
    static bool cppt_co_add_task(cppt_task_t&& task, unsigned tq_idx)
    {
        if (!g_task_queues[tq_idx]
                .try_enqueue(std::move(task))) {
            log_error("coroutine task queue %u full!", tq_idx);
            return false;
        }
        return true;
    }
    static void cppt_co_add_ptr(cor_sp co, unsigned tq_idx)
    {
        cppt_task_t task;
        task.m_co = co;
        cppt_co_add_task(std::move(task), tq_idx);
    }
    void cppt_co_add_c_ptr(cppt_co_c_sp c, unsigned tq_idx)
    {
        auto new_co = std::make_shared<cppt_co_t>(c);
        cppt_co_add_ptr(new_co, tq_idx);
    }

    static void cppt_co_add_sptr_f_before(cor_sp co,
                                          std::function<void()>& f_before,
                                          unsigned tq_idx)
    {
        cppt_task_t task;
        task.m_co = co;
        task.m_f_before_execution = f_before;
        cppt_co_add_task(std::move(task), tq_idx);
    }

    // Type implementation
    static void cppt_co_main_run_thread(co_executor_sp executor);
    void co_executor_t::start(co_executor_sp executor)
    {
        executor->m_turn_on = true;
        m_t = std::thread(cppt_co_main_run_thread, executor);
        m_t.detach();
    }

    void cppt_co_t::start_user_co()
    {
        bool expected = false;
        if (!m_co_started.compare_exchange_strong(expected, true)) {
            return;
        }

        *m_c = context::callcc([&](context::continuation && c) {
            /// Fixme: thread_local g_executor may cause bugs?
            g_executor->m_executor_c = std::move(c);
            m_user_co();
            m_co_stopped = true;
            cppt_co_wait_queue_ele_t ele;
            while (m_wait_cos.try_pop(ele)) {
                if (*(ele.c)) {
                    cppt_co_add_c_ptr(ele.c, ele.tq_idx);
                }
            }
            return std::move(g_executor->m_executor_c);
        });
    }

    void cppt_co_t::resume_user_co()
    {
        *m_c = m_c->resume();
    }

    bool cppt_co_t::is_started()
    {
        return m_co_started;
    }

    bool cppt_co_t::can_resume()
    {
        return *m_c ? true : false;
    }

    void cppt_co_t::join()
    {
        if (!m_co_started) {
            return;
        }
        if (m_co_stopped) {
            return;
        }
        auto ret_co = context::callcc([&, tq_idx(g_executor->m_tq_idx)](context::continuation && c) {
            auto caller_c = std::make_shared<context::continuation>(std::move(c));
            cppt_co_wait_queue_ele_t ele = { caller_c, tq_idx };
            if (!m_wait_cos.try_push(ele)) {
                /// Fixme: how to do then queue is full?
                log_error("m_wait_cos queue is full!!!");
                return std::move(g_executor->m_executor_c);
            }
            if (m_co_stopped) {
                if (*caller_c) {
                    return std::move(*caller_c);
                }
            }
            return std::move(g_executor->m_executor_c);
        });
        if (!ret_co) {
            // caller resume by self
            return;
        }
        g_executor->m_executor_c = std::move(ret_co);
    }


    // Interfaces
    cor_sp cppt_co_create0(std::function<void()> user_co)
    {
        cppt_task_t task;
        auto co = std::make_shared<cppt_co_t>(std::move(user_co));
        task.m_co = co;
        cppt_co_add_task_global(std::move(task));
        return co;
    }

    static void asio_thread(io_context& io_ctx)
    {
        util_thread_set_self_name("co_asio");
        boost::asio::io_context::work io_work(io_ctx);

        boost::asio::steady_timer timer(io_ctx);
        std::function<void(const boost::system::error_code&)> timer_cb = [&](const boost::system::error_code& ec){
            gs_time_wheel.tick();
            /// fixme: tick interval is not accurate, should adjust time for expires_after.
            timer.expires_after(boost::asio::chrono::seconds(1));
            timer.async_wait(timer_cb);
        };
        timer.async_wait(timer_cb);

        io_ctx.run();
        log_info("Asio io context quit!!");
    }

    static void cppt_co_main_run_thread(co_executor_sp executor)
    {
        char thr_name[16];
//        util_bind_thread_to_core(executor->m_tq_idx);
        snprintf(thr_name, sizeof(thr_name), "co_%02u", executor->m_tq_idx);
        util_thread_set_self_name(thr_name);

        g_executor = executor;

        const unsigned tq_idx = g_executor->m_tq_idx;
        std::mutex cond_lock;
        std::condition_variable cond_cv;
        bool notified = false;

        auto notify_handler =
                [&]()
                {
                    std::lock_guard lock(cond_lock);
                    notified = true;
                    cond_cv.notify_one();
                };

        auto cv_wait = [&](unsigned int timeout_ms) {
            {
                std::unique_lock<std::mutex> u_lock(cond_lock);
                cond_cv.wait_for(u_lock, std::chrono::milliseconds (timeout_ms), [&notified](){
                    if (notified) {
                        return true;
                    }
                    return false;
                });
                notified = false;
            }
            return g_run_flag;
        };

        cppt_task_t& g_cur_task_c = g_executor->m_cur_task_c;
        auto exec_co = [&](){
            g_executor->m_is_executing = true;
            auto co = g_cur_task_c.m_co;
            if (!co->is_started()) {
                co->start_user_co();
            } else if (co->can_resume()) {
                if (g_cur_task_c.m_f_before_execution) {
                    g_cur_task_c.m_f_before_execution();
                    g_cur_task_c.m_f_before_execution = nullptr;
                }
                co->resume_user_co();
            } else {
                g_cur_task_c.m_co = nullptr;
                g_executor->m_is_executing = false;
                return;
            }
            if (g_cur_task_c.m_f_after_execution) {
                g_cur_task_c.m_f_after_execution(g_cur_task_c.m_co);
                g_cur_task_c.m_f_after_execution = nullptr;
            } else {
                g_cur_task_c.m_co = nullptr;
            }
            g_executor->m_is_executing = false;
            g_executor->m_is_blocking = false;
        };

        std::function<bool(unsigned int)> wait_handler;
        if (gs_core_num > 1) {
            wait_handler = [&](unsigned int timeout_ms){
                // work stealing
                #if !defined(CPPT_COR_NO_WORKSTEAL) && (!(defined(_MSC_VER) && !defined(__INTEL_COMPILER)))
//                #if 1
                unsigned next_tq_idx = (tq_idx + 1) % gs_core_num;
                unsigned task_num = g_task_queues[next_tq_idx].get_size();
                if (task_num > 1) {
                    unsigned steal_task_num = std::min((task_num + 1) / 2, 10U);
                    cppt_task_t task;
                    unsigned i = 0;
                    for (; i < steal_task_num; i++) {
                        if (!g_task_queues[next_tq_idx].try_dequeue(task)) {
                            break;
                        }
                        if (!g_task_queues[tq_idx]
                                .try_enqueue_no_notify(std::move(task))) {
                            log_error("failed to enqueue task! tq idx: %u", tq_idx);
                            break;
                        }
                    }
                    if (0 != i) {
                        notified = false;
                        return g_run_flag;
                    }
                } else if (1 == task_num) {
                    if (g_task_queues[next_tq_idx].try_dequeue(g_cur_task_c)) {
                        exec_co();
                        notified = false;
                        return g_run_flag;
                    }
                }
                #endif

                return cv_wait(1000);
            };
        } else {
            wait_handler = cv_wait;
        }

        g_task_queues[tq_idx].set_handlers(notify_handler, std::move(wait_handler));

        while (g_executor->m_turn_on && g_run_flag) {
            if (g_task_queues[tq_idx].dequeue(g_cur_task_c)) {
                exec_co();
            }
        }
    }

    void co_run_impl()
    {
        // io and timer thread
        gs_tw_task_pool = std::make_shared<mempool>(4096, sizeof(time_wheel_task_t));
        expect_ret(gs_tw_task_pool);
        expect_ret(0 == gs_tw_task_pool->init());
        std::shared_ptr<void*> tw_task_pool_defer(nullptr, [](void*p){
            gs_tw_task_pool->deinit();
        });
        std::thread asio_thr{ asio_thread, std::ref(g_io_ctx) };
        asio_thr.detach();

        // prepare coroutine threads
        auto core_num = std::thread::hardware_concurrency();
        if (0 == core_num) {
            gs_core_num = 1;
        } else {
            gs_core_num = core_num;
        }
        g_task_queues.resize(gs_core_num);
        g_executors.resize(gs_core_num);
        for (unsigned i = 0; i < gs_core_num; i++) {
            g_executors[i] = std::make_shared<co_executor_t>(i);
            g_executors[i]->start(g_executors[i]);
        }

        struct executor_status_t {
            uint64_t blocking_start_ts_ms = 0;
        };
        uint64_t now_ms = util_now_ts_ms();
        uint64_t pre_ms = now_ms;
        const uint64_t timeout_ms = 1000;
        std::vector<executor_status_t> executor_status(gs_core_num);

        while (g_run_flag) {
            cppt_msleep(1000);
            now_ms = util_now_ts_ms();

            // replace blocking executor
#if !defined(CPPT_COR_NO_WORKSTEAL) && (!(defined(_MSC_VER) && !defined(__INTEL_COMPILER)))
            for (unsigned i = 0; i < gs_core_num; i++) {
                auto executor = g_executors[i];
                if (executor->m_is_executing) {
                    if (executor->m_is_blocking) {
                        if (timeout_ms < (now_ms - executor_status[i].blocking_start_ts_ms)) {
                            executor->m_turn_on = false;
                            g_executors[i] = std::make_shared<co_executor_t>(i);
                            g_executors[i]->start(g_executors[i]);
                        }
                    } else {
                        executor->m_is_blocking = true;
                        executor_status[i].blocking_start_ts_ms = now_ms;
                    }
                }
            }
#endif

//        if ((now_ms - pre_ms) > 5000) {
//            printf("------------\n");
//            for (unsigned i = 0; i < gs_core_num; i++) {
//                printf("tq[%02u] task num: %02u\n", i, g_task_queues[i].get_size());
//            }
//            printf("------------\n");
//            pre_ms = now_ms;
//        }
        }
    }

    boost::asio::io_context& cor_get_io_context_impl()
    {
        return g_io_ctx;
    }

    int co_yield_impl()
    {
        if (!g_executor->m_executor_c) {
            return -1;
        }
        auto wrap_func = [&, tq_idx(g_executor->m_tq_idx)](cor_sp co) {
            cppt_co_add_ptr(co, tq_idx);
        };
        g_executor->m_cur_task_c.m_f_after_execution = wrap_func;
        g_executor->m_executor_c = g_executor->m_executor_c.resume();
        return 0;
    }

    // ret: 0, ok; -1 coroutine error
    int co_yield_impl(
            const std::function<void(std::function<void(int result)>&& resume_f)>& wrapped_extern_func)
    {
        if (!g_executor->m_executor_c) {
            return -1;
        }
        int result = -1;
        auto wrap_func = [&, tq_idx(g_executor->m_tq_idx)](cor_sp co) {
            wrapped_extern_func([co, tq_idx, &result](int _result){
                /// Fixme: how to do when executing queue is full?
                result = _result;
                cppt_co_add_ptr(co, tq_idx);
            });
        };
        g_executor->m_cur_task_c.m_f_after_execution = wrap_func;
        g_executor->m_executor_c = g_executor->m_executor_c.resume();
        return result;
    }

    // ret: 0, ok; 1 timeout; -1 coroutine error
    int co_yield_impl(
            const std::function<void(std::function<void(int result)>&& resume_f)>& wrapped_extern_func,
            unsigned int timeout_ms,
            const std::function<void()>& f_cancel_operation)
    {
        if (0 == timeout_ms) {
            return co_yield_impl(wrapped_extern_func);
        }
        if (!g_executor->m_executor_c) {
            return -1;
        }
        bool is_timeout = false;
        int result = -1;

        auto tw_task_buf = gs_tw_task_pool->alloc();
        expect_ret_val(tw_task_buf, -1);
        auto tw_task = new (tw_task_buf) time_wheel_task_t(timeout_ms/1000);
        auto mutex_tw_task = std::make_shared<std::mutex>();

        auto wrap_func = [&, tq_idx(g_executor->m_tq_idx)](cor_sp co) {
            wrapped_extern_func([&, co, tq_idx](int _result){
                std::function<void()> f_before = [&, mutex_tw_task](){
                    // stop timer
                    std::lock_guard lock(*mutex_tw_task);
                    if (tw_task_buf) {
                        tw_task->cancel();
                    }
                };
                /// Fixme: how to do when executing queue is full?
                result = _result;
                cppt_co_add_sptr_f_before(co, f_before, tq_idx);
            });
            tw_task->set_cb([&, co, tq_idx](){
                std::function<void()> f_before = [&](){
                    is_timeout = true;
                    // cancel operation
                    if (f_cancel_operation) {
                        f_cancel_operation();
                    }
                };
                /// Fixme: how to do when executing queue is full?
                cppt_co_add_sptr_f_before(co, f_before, tq_idx);
            });
            gs_time_wheel.add_tw_task(tw_task, [&, mutex_tw_task](){
                std::lock_guard lock(*mutex_tw_task);
                gs_tw_task_pool->free(tw_task_buf);
                tw_task_buf = nullptr;
            });
        };

        g_executor->m_cur_task_c.m_f_after_execution = wrap_func;
        g_executor->m_executor_c = g_executor->m_executor_c.resume();

        return is_timeout ? 1 : result;
    }

    co_executor_sp cppt_get_cur_executor()
    {
        return g_executor;
    }
}

namespace cppt {

    // ret: 0, ok; -1 coroutine error
    int cor_yield()
    {
        return cppt_impl::co_yield_impl();
    }

    // ret: 0, ok; -1 coroutine error
    int cor_yield(
            const std::function<void(std::function<void(int result)>&& resume_f)>& wrapped_extern_func)
    {
        return cppt_impl::co_yield_impl(wrapped_extern_func);
    }

    // ret: 0, ok; 1 timeout
    int cor_yield(
            const std::function<void(std::function<void(int result)>&& resume_f)>& wrapped_extern_func,
            unsigned int timeout_ms,
            const std::function<void()>& f_cancel_operation)
    {
        return cppt_impl::co_yield_impl(wrapped_extern_func,
                                        timeout_ms,
                                        f_cancel_operation);
    }

    void cor_run()
    {
        cppt_impl::co_run_impl();
    }

    boost::asio::io_context& cor_get_io_context()
    {
        return cppt_impl::cor_get_io_context_impl();
    }
}
