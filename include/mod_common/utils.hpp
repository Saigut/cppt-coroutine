#ifndef CPPT_UTILS_H
#define CPPT_UTILS_H

#include <inttypes.h>

#include <string>
#include <functional>

#include <mod_common/os_compat.hpp>
#include "log.hpp"
#include "expect.hpp"


int util_bind_thread_to_core(unsigned int core_id);
void util_thread_set_self_name(std::string&& name);

uint64_t util_now_ts_ms();
uint64_t util_now_ts_us();
uint64_t util_now_ts_ns();

void util_printf_buf(uint8_t* buf, size_t size);


class util_send_data {
public:
    enum emSEND_DATA_STATE {
        emSEND_DATA_STATE_IDLE = 0,
        emSEND_DATA_STATE_STARTED,
        emSEND_DATA_STATE_FINISHED
    };

    int init(size_t buf_sz,
             size_t send_time_len_s,
             std::function<ssize_t(uint8_t*, size_t, uint8_t)> send_cb) {
        if (m_buf) { return 0; }
        expect_ret_val(0 != buf_sz, -1);
        m_buf = (uint8_t*)malloc(buf_sz);
        expect_ret_val(m_buf, -1);
        m_buf_sz = buf_sz;
        m_send_cb = send_cb;
        m_send_time_len_s = send_time_len_s;
        m_buf_read_pos = 0;
        return 0;
    }

    ~util_send_data() {
        if (!m_buf) { return; }
        m_buf_sz = 0;
        free(m_buf);
        m_buf = nullptr;
    }

    int send() {
        uint64_t cur_ts_ms;
        ssize_t sent_bytes;
        ssize_t want_send_bytes;
        uint64_t transport_speed_KiB;
        if (emSEND_DATA_STATE_FINISHED == m_state) {
            return -2;
        }
        if (emSEND_DATA_STATE_IDLE == m_state) {
            printf("Begin to send data!\n");
            m_start_ts_ms = util_now_ts_ms();
            m_last_ts_ms = m_start_ts_ms;
            m_state = emSEND_DATA_STATE_STARTED;
            cur_ts_ms = m_start_ts_ms;
        } else {
            cur_ts_ms = util_now_ts_ms();
        }

        if (cur_ts_ms > (m_start_ts_ms + m_send_time_len_s * 1000)) {
            m_fin_flag = 1;
        }

        if (m_buf_read_pos >= m_buf_sz) {
            m_buf_read_pos = 0;
        }

        want_send_bytes = m_buf_sz - m_buf_read_pos;
        sent_bytes = m_send_cb(m_buf + m_buf_read_pos,
                               want_send_bytes,
                               m_fin_flag);
        if (-2 == sent_bytes) {
            // again
            return 0;

        } else if (sent_bytes <= 0) {
            printf("send failed! sent_bytes: %zd, want send: %zu\n",
                   sent_bytes, want_send_bytes);
            return -1;

        } else {
            m_total_sent_bytes += sent_bytes;
            m_buf_read_pos += sent_bytes;
            if (m_fin_flag == 1) {
                transport_speed_KiB = (m_total_sent_bytes * 1000 /
                                      (cur_ts_ms - m_start_ts_ms)) / 1024;
                printf("Data sent! Average speed: %" PRIu64 " KiB/s\n", transport_speed_KiB);
                printf("Data size: %zu KiB\n", m_total_sent_bytes / 1024);
                printf("Spent time: %" PRIu64 "ms\n", cur_ts_ms - m_start_ts_ms);
                m_state = emSEND_DATA_STATE_FINISHED;
            }
            return 0;
        }
    }

    void print_speed()
    {
        uint64_t cur_ts_ms;
        uint64_t transport_speed_KiB;
        cur_ts_ms = util_now_ts_ms();
        transport_speed_KiB = ((m_total_sent_bytes - m_last_sent_bytes) * 1000 /
                               (cur_ts_ms - m_last_ts_ms)) / 1024;
        printf("Current speed: %" PRIu64 " KiB/s\n", transport_speed_KiB);
        m_last_sent_bytes = m_total_sent_bytes;
        m_last_ts_ms = cur_ts_ms;
    }

private:
    emSEND_DATA_STATE m_state = emSEND_DATA_STATE_IDLE;
    std::function<ssize_t(uint8_t*, size_t, uint8_t)> m_send_cb;

    uint8_t* m_buf = nullptr;
    size_t m_buf_sz = 0;
    size_t m_buf_read_pos = 0;

    size_t m_send_time_len_s = 5;
    uint8_t m_fin_flag = 0;

    size_t m_total_sent_bytes = 0;
    size_t m_last_sent_bytes = 0;

    uint64_t m_start_ts_ms = 0;
    uint64_t m_last_ts_ms = 0;
};

class util_recv_data {
public:
    enum emRECV_DATA_STATE {
        emRECV_DATA_STATE_IDLE = 0,
        emRECV_DATA_STATE_STARTED,
        emRECV_DATA_STATE_FINISHED
    };

    int init(size_t buf_sz) {
        if (m_buf) { return 0; }
        expect_ret_val(0 != buf_sz, -1);
        m_buf = (uint8_t*)malloc(buf_sz);
        expect_ret_val(m_buf, -1);
        m_buf_sz = buf_sz;
        m_buf_write_pos = 0;
        return 0;
    }

    ~util_recv_data() {
        if (!m_buf) { return; }
        m_buf_sz = 0;
        free(m_buf);
        m_buf = nullptr;
    }

    ssize_t read_in(uint8_t* data_buf, size_t data_sz, uint8_t fin) {
        ssize_t read_bytes;
        uint64_t cur_ts_ms;
        uint64_t transport_speed_KiB;

        if (0 == data_sz) {
            return -1;
        }

        if (emRECV_DATA_STATE_FINISHED == m_state) {
            return -2;
        }

        if (emRECV_DATA_STATE_IDLE == m_state) {
            printf("Begin to receive data!\n");
            m_start_ts_ms = util_now_ts_ms();
            m_last_ts_ms = m_start_ts_ms;
            m_state = emRECV_DATA_STATE_STARTED;
        }

        if (m_buf_write_pos >= m_buf_sz) {
            /* reuse buffer to recv data */
            m_buf_write_pos = 0;
        }

        size_t can_copy_buf_sz = m_buf_sz - m_buf_write_pos;
        if (can_copy_buf_sz >= data_sz) {
            memcpy(m_buf + m_buf_write_pos, data_buf, data_sz);
            read_bytes = data_sz;
        } else {
            memcpy(m_buf + m_buf_write_pos, data_buf, can_copy_buf_sz);
            read_bytes = can_copy_buf_sz;
        }

        m_total_recv_bytes += read_bytes;
        m_buf_write_pos += read_bytes;
        if (0 != fin) {
            cur_ts_ms = util_now_ts_ms();
            transport_speed_KiB = (m_total_recv_bytes * 1000 /
                                  (cur_ts_ms - m_start_ts_ms)) / 1024;
            printf("Data received! Average speed: %" PRIu64 " KiB/s\n", transport_speed_KiB);
            printf("Data size: %zu KiB\n", m_total_recv_bytes / 1024);
            printf("Spent time: %" PRIu64 "ms\n", cur_ts_ms - m_start_ts_ms);
            m_state = emRECV_DATA_STATE_FINISHED;
            return -1;
        }
        return read_bytes;
    }

    void print_speed()
    {
        uint64_t cur_ts_ms = 0;
        uint64_t transport_speed_KiB;
        cur_ts_ms = util_now_ts_ms();
        transport_speed_KiB = ((m_total_recv_bytes - m_last_recv_bytes) * 1000 /
                               (cur_ts_ms - m_last_ts_ms)) / 1024;
        printf("Current speed: %" PRIu64 " KiB/s\n", transport_speed_KiB);
        m_last_recv_bytes = m_total_recv_bytes;
        m_last_ts_ms = cur_ts_ms;
    }

private:
    emRECV_DATA_STATE m_state = emRECV_DATA_STATE_IDLE;

    uint8_t* m_buf = nullptr;
    size_t m_buf_sz = 0;
    size_t m_buf_write_pos = 0;

    size_t m_total_recv_bytes = 0;
    size_t m_last_recv_bytes = 0;

    uint64_t m_start_ts_ms = 0;
    uint64_t m_last_ts_ms = 0;
};


void cppt_sleep(unsigned ts_s);
void cppt_msleep(unsigned ts_ms);
void cppt_usleep(unsigned ts_us);
void cppt_nanosleep(unsigned ts_ns);


template<typename Function, typename Tuple, size_t ... I>
auto utils_call_with_variadic_arg(Function& f, Tuple& t, std::index_sequence<I ...>)
{
    return f(std::get<I>(t) ...);
}

template<typename Function, typename Tuple>
auto utils_call_with_variadic_arg(Function& f, Tuple& t)
{
    static constexpr auto size = std::tuple_size<Tuple>::value;
    return utils_call_with_variadic_arg(f, t, std::make_index_sequence<size>{});
}


#endif //CPPT_UTILS_H
