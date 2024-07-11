#ifndef CPP_TOOLKIT_MOD_CO_TIMER_H
#define CPP_TOOLKIT_MOD_CO_TIMER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <mod_common/expect.hpp>

namespace {
    using boost::asio::ip::tcp;
    using boost::asio::io_context;
    class cor_timer {
    public:
        explicit cor_timer(io_context& io_ctx)
        : m_io_ctx(io_ctx),
          m_timer(std::make_shared<boost::asio::deadline_timer>(io_ctx)) {}

        int wait_for(unsigned int ts_ms,
                     std::function<void(const std::error_code&)> to_cb) {
            m_timer->expires_from_now(boost::posix_time::milliseconds(ts_ms));
            m_timer->async_wait([to_cb](const boost::system::error_code& ec) {
                check_ec(ec, "timer async_wait");
                auto std_ec = ec ? -1 : 0;
                to_cb(std::make_error_code(std::errc::timed_out));
            });
            return 0;
        }
        int cancel() {
            m_timer->cancel();
            return 0;
        }

    private:
        io_context& m_io_ctx;
        std::shared_ptr<boost::asio::deadline_timer> m_timer;
    };
}

#endif //CPP_TOOLKIT_MOD_CO_TIMER_H
