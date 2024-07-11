#ifndef CPP_TOOLKIT_LOG_HPP
#define CPP_TOOLKIT_LOG_HPP

#include <stdio.h>
#include <string.h>

static const char *log_basename(char const *path)
{
#if defined(_WIN32)
    const char *s = strrchr(path, '\\');
#else
    const char *s = strrchr(path, '/');
#endif
    if (!s)
        return path;
    else
        return s + 1;
}

// green
#define log_info(msg, ...) printf("INFO [%s:%s:%d] " msg "\n", log_basename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
// gray
#define log_debug(msg, ...) printf("DEBUG [%s:%s:%d] " msg "\n", log_basename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
// red
#define log_error(msg, ...) printf("ERROR [%s:%s:%d] " msg "\n", log_basename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
// yellow
#define log_warn(msg, ...) printf("WARN [%s:%s:%d] " msg "\n", log_basename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)



#endif //CPP_TOOLKIT_LOG_HPP
