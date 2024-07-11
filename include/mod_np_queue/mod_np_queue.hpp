#ifndef CPP_TOOLKIT_MOD_NP_QUEUE_HPP
#define CPP_TOOLKIT_MOD_NP_QUEUE_HPP

#include <functional>
#include <mod_atomic_queue/atomic_queue.hpp>
#include <mod_coroutine/mod_cor_mutex.hpp>
#include <condition_variable>


// Notify to polling queue。多写单读
template <class ELE_T, unsigned SIZE = 2048>
class np_queue_t {
private:
    using np_queue_aq_t = atomic_queue::AtomicQueue2<ELE_T, SIZE>;
public:
    explicit np_queue_t() {}
    np_queue_t(std::function<void()>&& notify_handler,
               std::function<bool(unsigned int)>&& waiting_handler)
               : m_notify_handler(notify_handler),
                 m_waiting_handler(waiting_handler) {}
    np_queue_t(np_queue_t<ELE_T, SIZE>&& other) noexcept {
        m_is_notify_mode.store(other.m_is_notify_mode);
        m_notified.store(other.m_notified);
        m_notify_handler = std::move(other.m_notify_handler);
        m_waiting_handler = std::move(other.m_waiting_handler);
        ELE_T tmp_ele;
        while (other.m_queue.try_pop(tmp_ele)) {
            m_queue.try_push(tmp_ele);
        }
    }
    void set_handlers(std::function<void()> notify_handler,
                      std::function<bool(unsigned int)> waiting_handler);
    void set_alloc_handlers(std::function<ELE_T()>&& ele_alloc_handler,
                            std::function<void(ELE_T)>&& ele_free_handler);
    bool enqueue(ELE_T&& p);
    bool try_enqueue(ELE_T&& p);
    bool try_enqueue_no_notify(ELE_T&& p);
    bool dequeue(ELE_T& p);
    bool dequeue(ELE_T& p, unsigned int timeout_ms);
    bool try_dequeue(ELE_T& p);

    ELE_T alloc_ele();
    void free_ele(ELE_T ele);

    unsigned get_size();

protected:
    int notify();

    np_queue_aq_t m_queue;

    std::atomic<bool> m_is_notify_mode = true;
    std::atomic<bool> m_notified = false;

    std::function<void()> m_notify_handler = nullptr;

    // 读到队列为空时即调用 m_waiting_handler。不可为 null
    // ret: true, 继续 dequeue; false, 不 dequeue 了。
    std::function<bool(unsigned int timeout_ms)> m_waiting_handler = nullptr;

    std::function<ELE_T()> m_ele_alloc_handler = nullptr;
    std::function<void(ELE_T)> m_ele_free_handler = nullptr;
};

template <class ELE_T, unsigned SIZE>
void np_queue_t<ELE_T, SIZE>::set_handlers(
        std::function<void()> notify_handler,
        std::function<bool(unsigned int)> waiting_handler)
{
    m_notify_handler = notify_handler;
    m_waiting_handler = waiting_handler;
}

template<class ELE_T, unsigned int SIZE>
void np_queue_t<ELE_T, SIZE>::set_alloc_handlers(
        std::function<ELE_T()>&& ele_alloc_handler,
        std::function<void(ELE_T)>&& ele_free_handler)
{
    m_ele_alloc_handler = ele_alloc_handler;
    m_ele_free_handler = ele_free_handler;
}

template <class ELE_T, unsigned SIZE>
int np_queue_t<ELE_T, SIZE>::notify()
{
    // 检查模式。notify 模式则通知
    if (m_is_notify_mode) {
//        if (!m_notified) {
//            // notify
//            if (m_notify_handler) {
//                // notify handler 需要是线程安全的
//                m_notify_handler();
//            } else {
//                return -1;
//            }
//            m_notified = true;
//        }
        if (m_notify_handler) {
            // notify handler 需要是线程安全的
            m_notify_handler();
        } else {
            return -1;
        }
        m_notified = true;
    }
    return 0;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::enqueue(ELE_T&& p)
{
    // 1. 写入
    m_queue.push(p);
    // 2. 通知
    if (0 != notify()) {
        return true;
    }
    return true;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::try_enqueue(ELE_T&& p)
{
    // 1. 写入
    if (!m_queue.try_push(p)) {
        return false;
    }
    // 2. 检查模式。notify 模式则通知。
    if (0 != notify()) {
        return true;
    }
    return true;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::try_enqueue_no_notify(ELE_T&& p)
{
    if (!m_queue.try_push(p)) {
        return false;
    }
    return true;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::dequeue(ELE_T& p)
{
    if (m_queue.try_pop(p)) {
        return true;
    }

    m_is_notify_mode = true;
    do {
        m_notified = false;
        if (m_queue.try_pop(p)) {
            m_is_notify_mode = false;
            return true;
        }
        if (!m_waiting_handler) {
            m_is_notify_mode = false;
            return false;
        }
    } while(m_waiting_handler(0));

    m_is_notify_mode = false;
    return false;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::dequeue(ELE_T& p, unsigned int timeout_ms)
{
    if (m_queue.try_pop(p)) {
        return true;
    }

    m_is_notify_mode = true;
    do {
        m_notified = false;
        if (m_queue.try_pop(p)) {
            m_is_notify_mode = false;
            return true;
        }
        if (!m_waiting_handler) {
            m_is_notify_mode = false;
            return false;
        }
    } while(m_waiting_handler(timeout_ms));

    m_is_notify_mode = false;
    return false;
}

template <class ELE_T, unsigned SIZE>
bool np_queue_t<ELE_T, SIZE>::try_dequeue(ELE_T& p)
{
    if (m_queue.try_pop(p)) {
        return true;
    }
    return false;
}

template<class ELE_T, unsigned int SIZE>
ELE_T np_queue_t<ELE_T, SIZE>::alloc_ele()
{
    return m_ele_alloc_handler();
}

template<class ELE_T, unsigned int SIZE>
void np_queue_t<ELE_T, SIZE>::free_ele(ELE_T ele)
{
    return m_ele_free_handler(ele);
}

template <class ELE_T, unsigned SIZE>
unsigned np_queue_t<ELE_T, SIZE>::get_size()
{
    return m_queue.was_size();
}

#define NP_QUEUE_DEF_SZ 10240
template <class ELE_T, unsigned SIZE = NP_QUEUE_DEF_SZ>
class np_queue_cor_t {
public:
    explicit np_queue_cor_t();
    void set_handlers(std::function<void()> notify_handler,
                      std::function<bool()> waiting_handler);
    void set_alloc_handlers(std::function<ELE_T()>&& ele_alloc_handler,
                            std::function<void(ELE_T)>&& ele_free_handler);
    void set_thr_nw_handlers_thr_r_cor_w();
    void set_thr_nw_handlers_cor_r_thr_w();
    bool enqueue(ELE_T&& p);
    bool try_enqueue(ELE_T&& p);
    bool dequeue(ELE_T& p);
    bool dequeue(ELE_T& p, unsigned int timeout_ms);
    bool try_dequeue(ELE_T& p);

    ELE_T alloc_ele();
    void free_ele(ELE_T ele);

    unsigned get_size();

private:
    std::function<void()> get_notify_handler_cor();
    std::function<bool(unsigned int)> get_wait_handler_cor();
    std::function<void()> get_notify_handler_cor_r_thr_w();
    std::function<bool(unsigned int)> get_wait_handler_cor_r_thr_w();
    std::function<void()> get_notify_handler_thr_r_cor_w();
    std::function<bool(unsigned int)> get_wait_handler_thr_r_cor_w();

    np_queue_t<ELE_T, SIZE> m_q;
    cppt::cor_mutex_t m_cor_mutex;
    std::mutex m_thr_mutex;
    std::condition_variable m_thr_cond_cv;
    std::function<void(int)> m_resume_f = nullptr;
    bool m_notified = false;
};

template<class ELE_T, unsigned int SIZE>
bool np_queue_cor_t<ELE_T, SIZE>::try_enqueue(ELE_T&& p)
{
    return m_q.try_enqueue(std::move(p));
}

template<class ELE_T, unsigned int SIZE>
void np_queue_cor_t<ELE_T, SIZE>::set_thr_nw_handlers_cor_r_thr_w()
{
    m_q.set_handlers(get_notify_handler_cor_r_thr_w(), get_wait_handler_cor_r_thr_w());
}

template<class ELE_T, unsigned int SIZE>
std::function<void()> np_queue_cor_t<ELE_T, SIZE>::get_notify_handler_cor_r_thr_w()
{
    return [this](){
        m_notified = true;
        m_thr_mutex.lock();
        if (m_resume_f) {
            m_resume_f(0);
            m_resume_f = nullptr;
        }
        m_thr_mutex.unlock();
    };
}

template<class ELE_T, unsigned int SIZE>
std::function<bool(unsigned int)> np_queue_cor_t<ELE_T, SIZE>::get_wait_handler_cor_r_thr_w()
{
    return [this](unsigned int timeout_ms){
        auto wrap_func = [this](std::function<void(int)>&& resume_f) {
            m_thr_mutex.lock();
            m_resume_f = std::move(resume_f);
            if (m_notified) {
                m_resume_f(0);
                m_resume_f = nullptr;
            }
            m_thr_mutex.unlock();
        };
        auto timeout_func = [](){};
        int ret = cppt::cor_yield(wrap_func, timeout_ms, timeout_func);
        if (1 == ret) {
            // timeout
            m_notified = false;
            return false;
        }
        m_notified = false;
        return true;
    };
}

template<class ELE_T, unsigned int SIZE>
void np_queue_cor_t<ELE_T, SIZE>::set_thr_nw_handlers_thr_r_cor_w()
{
    m_q.set_handlers(get_notify_handler_thr_r_cor_w(), get_wait_handler_thr_r_cor_w());
}

template<class ELE_T, unsigned int SIZE>
std::function<void()> np_queue_cor_t<ELE_T, SIZE>::get_notify_handler_cor()
{
    return [this](){
        m_notified = true;
        m_cor_mutex.lock();
        if (m_resume_f) {
            m_resume_f(0);
            m_resume_f = nullptr;
        }
        m_cor_mutex.unlock();
    };
}

template<class ELE_T, unsigned int SIZE>
std::function<bool(unsigned int)> np_queue_cor_t<ELE_T, SIZE>::get_wait_handler_cor()
{
    return [this](unsigned int timeout_ms){
        m_cor_mutex.lock();
        auto wrap_func = [&](std::function<void(int)> resume_f) {
            m_resume_f = std::move(resume_f);
            if (m_notified) {
                m_resume_f(0);
                m_resume_f = nullptr;
            }
            m_cor_mutex.unlock();
        };
        auto timeout_func = [](){};
        int ret = cppt::cor_yield(wrap_func, timeout_ms, timeout_func);
        if (1 == ret) {
            // timeout
            m_notified = false;
            return false;
        }
        m_notified = false;
        return true;
    };
}

template<class ELE_T, unsigned int SIZE>
std::function<void()> np_queue_cor_t<ELE_T, SIZE>::get_notify_handler_thr_r_cor_w()
{
    return [this](){
        std::lock_guard lock(m_thr_mutex);
        m_notified = true;
        m_thr_cond_cv.notify_one();
    };
}

template<class ELE_T, unsigned int SIZE>
std::function<bool(unsigned int)> np_queue_cor_t<ELE_T, SIZE>::get_wait_handler_thr_r_cor_w()
{
    return [this](unsigned int timeout_ms){
        {
            if (0 == timeout_ms) {
                timeout_ms = 3000;
            }
            std::unique_lock<std::mutex> u_lock(m_thr_mutex);
            m_thr_cond_cv.wait_for(u_lock, std::chrono::milliseconds(timeout_ms), [this](){
                if (m_notified) {
                    return true;
                }
                return false;
            });
            m_notified = false;
        }
        return true;
    };
}

template<class ELE_T, unsigned int SIZE>
np_queue_cor_t<ELE_T, SIZE>::np_queue_cor_t() {
    m_q.set_handlers(get_notify_handler_cor(), get_wait_handler_cor());
}

template<class ELE_T, unsigned int SIZE>
void np_queue_cor_t<ELE_T, SIZE>::set_handlers(std::function<void()> notify_handler,
                                               std::function<bool()> waiting_handler)
{
    m_q.set_handlers(notify_handler, waiting_handler);
}

template<class ELE_T, unsigned int SIZE>
void np_queue_cor_t<ELE_T, SIZE>::set_alloc_handlers(std::function<ELE_T()>&& ele_alloc_handler,
                                                     std::function<void(ELE_T)>&& ele_free_handler)
{
    m_q.set_alloc_handlers(std::move(ele_alloc_handler), std::move(ele_free_handler));
}

template<class ELE_T, unsigned int SIZE>
bool np_queue_cor_t<ELE_T, SIZE>::enqueue(ELE_T&& p)
{
    return m_q.enqueue(std::move(p));
}

template<class ELE_T, unsigned int SIZE>
bool np_queue_cor_t<ELE_T, SIZE>::dequeue(ELE_T& p)
{
    return m_q.dequeue(p);
}

template<class ELE_T, unsigned int SIZE>
bool np_queue_cor_t<ELE_T, SIZE>::dequeue(ELE_T& p, unsigned int timeout_ms)
{
    return m_q.dequeue(p, timeout_ms);
}

template<class ELE_T, unsigned int SIZE>
bool np_queue_cor_t<ELE_T, SIZE>::try_dequeue(ELE_T& p)
{
    return m_q.try_dequeue(p);
}

template<class ELE_T, unsigned int SIZE>
ELE_T np_queue_cor_t<ELE_T, SIZE>::alloc_ele()
{
    return m_q.alloc_ele();
}

template<class ELE_T, unsigned int SIZE>
void np_queue_cor_t<ELE_T, SIZE>::free_ele(ELE_T ele)
{
    m_q.free_ele(ele);
}

template<class ELE_T, unsigned int SIZE>
unsigned np_queue_cor_t<ELE_T, SIZE>::get_size()
{
    return m_q.get_size();
}

#endif //CPP_TOOLKIT_MOD_NP_QUEUE_HPP
