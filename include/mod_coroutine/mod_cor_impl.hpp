#ifndef CPP_TOOLKIT_MOD_COR_IMPL_HPP
#define CPP_TOOLKIT_MOD_COR_IMPL_HPP

#include <functional>
#include <thread>
#include <mod_common/utils.hpp>
#include <boost/context/continuation.hpp>
#include <mod_atomic_queue/atomic_queue.hpp>


//#define CPPT_COR_NO_WORKSTEAL

namespace cppt_impl {
    using cppt_co_c_sp = std::shared_ptr<boost::context::continuation>;
    struct cppt_co_wait_queue_ele_t {
        cppt_co_c_sp c;
        unsigned tq_idx;
    };
    using cppt_co_wait_queue_t = atomic_queue::AtomicQueue2<cppt_co_wait_queue_ele_t, 100>;

    class cppt_co_t {
    public:
        explicit cppt_co_t(std::function<void()> user_co)
                : m_c(std::make_shared<boost::context::continuation>()),
                  m_user_co(std::move(user_co)) {}
        explicit cppt_co_t(cppt_co_c_sp c)
                : m_c(c), m_co_started(true) {}

        bool is_started();
        bool can_resume();

        void start_user_co();
        void resume_user_co();

        void join();

    private:
        cppt_co_c_sp m_c;
        std::function<void()> m_user_co;
        std::atomic<bool> m_co_started = false;
        std::atomic<bool> m_co_stopped = false;
        cppt_co_wait_queue_t m_wait_cos;
    };
    using cor_sp = std::shared_ptr<cppt_co_t>;


    cor_sp cppt_co_create0(std::function<void()> user_co);


    class cppt_task_t {
    public:
        cor_sp m_co = nullptr;
        std::function<void()> m_f_before_execution = nullptr;
        std::function<void(cor_sp)> m_f_after_execution = nullptr;
    };

    class co_executor_t;
    using co_executor_sp = std::shared_ptr<co_executor_t>;
    class co_executor_t {
    public:
        co_executor_t() = default;
        explicit co_executor_t(unsigned tq_idx) : m_tq_idx(tq_idx) {}
        void start(co_executor_sp executor);

        bool m_turn_on = false;
        bool m_is_executing = false;
        bool m_is_blocking = false;

        unsigned m_tq_idx = 0;
        boost::context::continuation m_executor_c;
        cppt_task_t m_cur_task_c;
        std::thread m_t;
    };

    co_executor_sp cppt_get_cur_executor();
    void cppt_co_add_c_ptr(cppt_co_c_sp c, unsigned tq_idx);


    template<typename Function, typename Tuple, size_t ... I>
    auto co_call_with_variadic_arg(Function& f, Tuple& t, std::index_sequence<I ...>)
    {
        return f(std::get<I>(t) ...);
    }

    template<typename Function, typename Tuple>
    auto co_call_with_variadic_arg(Function& f, Tuple& t)
    {
        static constexpr auto size = std::tuple_size<Tuple>::value;
        return utils_call_with_variadic_arg(f, t, std::make_index_sequence<size>{});
    }

}

#endif //CPP_TOOLKIT_MOD_COR_IMPL_HPP
