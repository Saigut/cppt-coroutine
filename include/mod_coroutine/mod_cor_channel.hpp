#ifndef CPP_TOOLKIT_MOD_COR_CHANNEL_HPP
#define CPP_TOOLKIT_MOD_COR_CHANNEL_HPP

#include <mod_np_queue/mod_np_queue.hpp>

#include "mod_cor.hpp"
#include "mod_cor_mutex.hpp"


namespace cppt {

    template <class ELE_T, unsigned SIZE = NP_QUEUE_DEF_SZ>
    class cor_channel {
    public:
        explicit cor_channel();
        bool read(ELE_T& msg);
        bool read(ELE_T& msg, unsigned int timeout_ms);
        bool write(ELE_T&& msg);

    private:
        np_queue_cor_t<ELE_T, SIZE> m_q;
    };

    template<class ELE_T, unsigned int SIZE>
    cor_channel<ELE_T, SIZE>::cor_channel() {}

    template<class ELE_T, unsigned int SIZE>
    bool cor_channel<ELE_T, SIZE>::read(ELE_T& msg) {
        return m_q.dequeue(msg);
    }

    template<class ELE_T, unsigned int SIZE>
    bool cor_channel<ELE_T, SIZE>::read(ELE_T& msg, unsigned int timeout_ms) {
        return m_q.dequeue(msg, timeout_ms);
    }

    template<class ELE_T, unsigned int SIZE>
    bool cor_channel<ELE_T, SIZE>::write(ELE_T&& msg) {
        return m_q.try_enqueue(std::move(msg));
    }
}

#endif //CPP_TOOLKIT_MOD_COR_CHANNEL_HPP
