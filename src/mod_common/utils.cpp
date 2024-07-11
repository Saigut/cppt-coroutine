#include <mod_common/utils.hpp>

#include <stdint.h>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <processthreadsapi.h>
    #include <boost/locale.hpp>
#else
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

#ifdef __APPLE__
    #include <mach/thread_policy.h>
    #include <mach/thread_act.h>
#endif


int util_bind_thread_to_core(unsigned int core_id)
{
#if defined(_WIN32)
    if (0 == SetThreadAffinityMask(GetCurrentThread(), 0x1 << core_id)) {
        return -1;
    } else {
        return 0;
    }
#elif defined(__linux__)
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id >= num_cores) {
        return -1;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (0 != pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        return -1;
    } else {
        return 0;
    }
#elif defined(__APPLE__)
//    thread_affinity_policy_data_t policy_data = { (int)core_id + 1 };
//    thread_policy_set(pthread_mach_thread_np(pthread_self()),
//                      THREAD_AFFINITY_POLICY,
//                      (thread_policy_t)&policy_data,
//                      THREAD_AFFINITY_POLICY_COUNT);
    return 0;
#else
    return -1;
#endif
}

void util_thread_set_self_name(std::string&& name)
{
#if defined(_WIN32)
    SetThreadDescription(GetCurrentThread(), std::wstring(boost::locale::conv::utf_to_utf<wchar_t>(name.c_str())).c_str());
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), name.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name.c_str());
#else
#endif
}

uint64_t util_now_ts_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t util_now_ts_us()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t util_now_ts_ns()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

void util_printf_buf(uint8_t* buf, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

void cppt_sleep(unsigned ts_s)
{
    std::this_thread::sleep_for(std::chrono::seconds(ts_s));
}

void cppt_msleep(unsigned ts_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ts_ms));
}

void cppt_usleep(unsigned ts_us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(ts_us));
}

void cppt_nanosleep(unsigned ts_ns)
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(ts_ns));
}
