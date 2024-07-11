#ifndef CPP_TOOLKIT_OS_COMPAT_HPP
#define CPP_TOOLKIT_OS_COMPAT_HPP

#if defined(_MSC_VER)
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
#endif

#endif //CPP_TOOLKIT_OS_COMPAT_HPP
