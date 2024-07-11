#ifndef CPP_TOOLKIT_EXPECT_HPP
#define CPP_TOOLKIT_EXPECT_HPP

#include "log.hpp"


#define expect(_exp) \
do { \
    if (!(_exp)) { log_error("%s check failed!", #_exp); } \
} while (0)

#define expect_ret(_exp) \
do { \
    if (!(_exp)) { log_error("%s check failed!", #_exp); return; } \
} while (0)

#define expect_ret_val(_exp, _val) \
do { \
    if (!(_exp)) { log_error("%s check failed!", #_exp); return _val; } \
} while (0)

#define expect_goto(_exp, _goto_flag) \
do { \
    if (!(_exp)) { log_error("%s check failed!", #_exp); goto _goto_flag; } \
} while (0)

#define expect_ret_p(_exp, _prompt_fmt, ...) \
do { \
    if (!(_exp)) {\
        log_error("%s check failed! prompt: " _prompt_fmt, \
            #_exp, ##__VA_ARGS__); \
        return; \
    } \
} while (0)

#define expect_ret_val_p(_exp, _val, _prompt_fmt, ...) \
do { \
    if (!(_exp)) {\
        log_error("%s check failed! prompt: " _prompt_fmt, \
            #_exp, ##__VA_ARGS__); \
        return _val; \
    } \
} while (0)

#define expect_goto_p(_exp, _goto_flag, _prompt_fmt, ...) \
do { \
    if (!(_exp)) {\
        log_error("%s check failed! prompt: " _prompt_fmt, \
            #_exp, ##__VA_ARGS__); \
        goto _goto_flag; \
    } \
} while (0)

#define check_ec(_ec, _prompt) \
do { \
    if (_ec) { \
        log_error("error: %s, %s", \
            _prompt, ec.message().c_str()); \
    } \
} while (0)

#define check_ec_ret(_ec, _prompt) \
do { \
    if (_ec) { \
        log_error("error: %s, %s", \
            _prompt, ec.message().c_str()); \
        return; \
    } \
} while (0)

#define check_ec_ret_val(_ec, _val, _prompt) \
do { \
    if (_ec) { \
        log_error("error: %s, %s", \
            _prompt, ec.message().c_str()); \
        return _val; \
    } \
} while (0)

#define check_ec_goto(_ec, _goto_flag, _prompt) \
do { \
    if (_ec) { \
        log_error("error: %s, %s", \
            _prompt, ec.message().c_str()); \
        goto _goto_flag; \
    } \
} while (0)

#endif //CPP_TOOLKIT_EXPECT_HPP
