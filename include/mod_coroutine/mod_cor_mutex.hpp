#ifndef CPP_TOOLKIT_MOD_COR_MUTEX_HPP
#define CPP_TOOLKIT_MOD_COR_MUTEX_HPP

#include <atomic>
#include <mod_coroutine/mod_cor.hpp>


namespace cppt {

    class cor_mutex_t {
    public:
        bool lock();
        bool try_lock();
        void unlock();

    private:
        struct cppt_co_mutex_wait_queue_ele_t {
            std::shared_ptr<boost::context::continuation> c;
            unsigned tq_idx;
        };
        using cppt_co_mutex_wait_queue_t = atomic_queue::AtomicQueue2<cppt_co_mutex_wait_queue_ele_t, 1024>;

        std::atomic<bool> m_lock_acquired = false;
        cppt_co_mutex_wait_queue_t m_wait_cos;
    };
}

#endif //CPP_TOOLKIT_MOD_COR_MUTEX_HPP
