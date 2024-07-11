#include <iostream>
#include <app_test_http_client/app_test_http_client.hpp>


static int program_main(int argc, char** argv)
{
    int ret = 0;
    ret = app_test_http_client(argc, argv);
    return ret;
}

int main(int argc, char** argv)
{
    int ret = -1;

    try {
        ret = program_main(argc, argv);
    } catch (const std::overflow_error& e) {
        std::cout << "overflow_error: " << e.what();
    } catch (const std::runtime_error& e) {
        std::cout << "runtime_error: " << e.what();
    } catch (const std::exception& e) {
        std::cout << "exception: " << e.what();
    } catch (...) {
        std::cout << "unknown exception!";
    }

    return ret;
}
