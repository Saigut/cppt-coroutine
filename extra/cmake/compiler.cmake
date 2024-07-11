include_guard()

include(common_var)

macro(compiler_set_compilation_options)
    # Compiler
    ## Firstly, chose generator, this is chosing build toochain. It's like only can chose generator on cmake command line.

    message("Number of logic cores of hardware: " ${GV_LCORES})

    ## Which compiler
    set(V_conan_compiler "")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Using Clang
        set(CLANG true)
        set(V_conan_compiler "clang")
        add_definitions(-D_CPPT_CPL_CLANG)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Using GCC
        set(GCC true)
        set(V_conan_compiler "gcc")
        add_definitions(-D_CPPT_CPL_GCC)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        # Using Intel C++
        set(INTEL true)
        set(V_conan_compiler "intel")
        add_definitions(-D_CPPT_CPL_INTEL)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Using Visual Studio C++
        set(MSVC true)
        set(V_conan_compiler "msvc")
        add_definitions(-D_CPPT_CPL_MSVC)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        # Using Apple Clang
        set(AppleClang true)
        set(V_conan_compiler "apple-clang")
        add_definitions(-D_CPPT_CPL_APPLE_CLANG)
    endif()
    message("Using C Compiler: " ${CMAKE_C_COMPILER_ID})
    message("Using CXX Compiler: " ${CMAKE_CXX_COMPILER_ID})

    ## Which OS
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        set(OS_LINUX true)
        add_definitions(-D_CPPT_OS_LINUX)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(OS_WINDOWS true)
        add_definitions(-D_CPPT_OS_WINDOWS)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        set(OS_DARWIN true)
        add_definitions(-D_CPPT_OS_DARWIN)
    endif()
    message("OS: " ${CMAKE_HOST_SYSTEM_NAME})

    ## Settings for different OSes and compilers
    add_definitions(-DBOOST_ALL_NO_LIB)
    if (OS_LINUX)
        set(PLATFORM_LINK_LIB pthread dl m)
        if (GCC)
#            set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG")
#            set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=broadwell")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=broadwell")
            add_definitions(-D_GNU_SOURCE)
        endif()
    endif ()
    if (GCC)
        # Force GCC to use the old ABI version. In case dependency libraries were built using old ABI version.
#        add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
    if (OS_WINDOWS)
        # _WIN32_WINNT for boost
        add_definitions(-D_WIN32_WINNT=0x0601)
        add_definitions(-DJSON_C_HAVE_INTTYPES_H=1) # for json-c
        if (MSVC)
            add_definitions(-D_CRT_SECURE_NO_WARNINGS)
            add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
            add_definitions(-DNCURSES_STATIC)

            # for boost.context. https://www.boost.org/doc/libs/1_81_0/libs/context/doc/html/context/requirements.html
            string(REPLACE " /EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs /MP /O2")
#            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs /MP")

#            string(REPLACE " /O2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
#            string(REPLACE " /Ob2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
#            string(REPLACE " /O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#            string(REPLACE " /Ob2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ot")
#            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot")
#            set(PLATFORM_LINK_LIB ${PLATFORM_LINK_LIB} Kernel32)
        endif ()
        if (CLANG)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 /EHa")
        endif ()
        if (GCC)
            set(PLATFORM_LINK_LIB ${PLATFORM_LINK_LIB} ws2_32)
        endif ()
    endif()

    ## Standard
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_CXX_STANDARD 17)

    string(REPLACE "." ";" VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
    list(GET VERSION_LIST 0 CMAKE_CXX_COMPILER_VERSION_MAJOR)
    list(GET VERSION_LIST 1 CMAKE_CXX_COMPILER_VERSION_MINOR)
    list(GET VERSION_LIST 2 CMAKE_CXX_COMPILER_VERSION_PATCH)
#    message("CMAKE_CXX_COMPILER_VERSION_MAJOR: ${CMAKE_CXX_COMPILER_VERSION_MAJOR}")
#    message("CMAKE_CXX_COMPILER_VERSION_MINOR: ${CMAKE_CXX_COMPILER_VERSION_MINOR}")
#    message("CMAKE_CXX_COMPILER_VERSION_PATCH: ${CMAKE_CXX_COMPILER_VERSION_PATCH}")
    message("CMAKE_C_COMPILER_VERSION: ${CMAKE_C_COMPILER_VERSION}")
    message("CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
    message("CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
    message("CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
    message("CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
    message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
    message("CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
    message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
    message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endmacro()
