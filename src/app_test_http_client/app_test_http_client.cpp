#include <app_test_http_client/app_test_http_client.hpp>
#include <mod_coroutine/mod_cor.hpp>
#include <app_coroutine/io_context_pool.hpp>
#include <mod_coroutine/mod_cor_net.hpp>


static void asio_pool_thread(io_context_pool& io_ctx_pool)
{
    io_ctx_pool.run();
    log_info("Asio io context quit!!");
}
static void my_co_net(io_context_pool& io_ctx_pool)
{
    cppt::cor_tcp_socket_builder builder(io_ctx_pool.get_io_context());
    auto peer_socket = builder.connect("127.0.0.1", 10666);
    if (!peer_socket) {
        log_error("connect failed!");
        return;
    }

    char send_buf[4096];
    char recv_buf[4096] = { 0 };
    ssize_t ret_recv;
    snprintf(send_buf, sizeof(send_buf), "GET / HTTP/1.1\r\n"
                                         "Host: 127.0.0.1\r\n"
                                         "Connection: keep-alive\r\n"
                                         "User-Agent: curl/7.58.0\r\n"
                                         "Accept: */*\r\n\r\n");
    // Send
    expect_goto(peer_socket->write((uint8_t*)send_buf, std::strlen(send_buf)), func_return);

    // Recv
    ret_recv = peer_socket->read_some((uint8_t*)recv_buf, sizeof(recv_buf));
    expect_goto(ret_recv > 0, func_return);

    recv_buf[ret_recv < sizeof(recv_buf) ? ret_recv : sizeof(recv_buf) - 1] = 0;
    log_info("received:\n%s!", recv_buf);

    func_return:
    return;
}

static void main_co(io_context_pool& io_ctx_pool)
{
    for (int i = 0; i < 50; i++) {
        cppt::cor_create(my_co_net, std::ref(io_ctx_pool));
//        auto id = cppt_co_awaitable_create(my_co_net, std::ref(io_ctx));
//        cppt_co_await(id);
    }
}

int app_test_http_client(int argc, char** argv)
{
    io_context_pool io_ctx_pool{4};
    std::thread asio_pool_thr{asio_pool_thread, std::ref(io_ctx_pool)};
    asio_pool_thr.detach();

    cppt::cor_create(main_co, std::ref(io_ctx_pool));
    cppt::cor_run();
    return 0;
}