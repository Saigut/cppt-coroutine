#include <mod_coroutine/mod_cor_mutex.hpp>
#include <boost/context/continuation.hpp>

namespace cppt {
    namespace context = boost::context;

    bool cor_mutex_t::lock()
    {
        while (true) {
            bool expected = false;
            if (m_lock_acquired.compare_exchange_strong(expected, true)) {
                return true;
            }

            auto ret_co = context::callcc([&](context::continuation && c) {
                auto executor = cppt_impl::cppt_get_cur_executor();
                auto caller_c = std::make_shared<context::continuation>(std::move(c));
                cppt_co_mutex_wait_queue_ele_t ele = { caller_c, executor->m_tq_idx };
                if (!m_wait_cos.try_push(ele)) {
                    /// Fixme: how to do then queue is full?
                    log_error("m_wait_cos queue is full!!!");
                    return std::move(executor->m_executor_c);
                }
                expected = false;
                if (m_lock_acquired.compare_exchange_strong(expected, true)) {
                    if (*caller_c) {
                        ele.c = nullptr;
                        return std::move(*caller_c);
                    } else {
                        log_error("caller_c wrongly became invalid!");
                    }
                }
                return std::move(executor->m_executor_c);
            });

            if (!ret_co) {
                // caller resume by self
                return true;
            } else {
                // resume by scheduler, try to acquire lock again
                cppt_impl::cppt_get_cur_executor()->m_executor_c = std::move(ret_co);
            }
        }
    }

    bool cor_mutex_t::try_lock()
    {
        bool expected = false;
        return m_lock_acquired.compare_exchange_strong(expected, true);
    }

    void cor_mutex_t::unlock()
    {
        bool expected = true;
        if (!m_lock_acquired.compare_exchange_strong(expected, false)) {
            log_warn("mutex not locked?");
        }
        cppt_co_mutex_wait_queue_ele_t ele;
        while (m_wait_cos.try_pop(ele)) {
            if (ele.c && *(ele.c)) {
                cppt_impl::cppt_co_add_c_ptr(ele.c, ele.tq_idx);
                break;
            }
        }
    }
}
