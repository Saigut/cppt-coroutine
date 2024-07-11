#include <iostream>
#include <app_coroutine/app_coroutine.hpp>


static int program_main(int argc, char** argv)
{
    int ret = 0;
    ret = app_coroutine(argc, argv);
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
