include_guard()

# About compiler
set(CLANG false)
set(GCC false)
set(INTEL false)
set(MSVC false)
set(AppleClang false)

# About OS
set(OS_LINUX false)
set(OS_WINDOWS false)
set(OS_DARWIN false)

# Logic cores
cmake_host_system_information(RESULT GV_LCORES QUERY NUMBER_OF_LOGICAL_CORES)
math(EXPR GV_LCORES_PLUS1 "${GV_LCORES} + 1")
set(CMAKE_BUILD_PARALLEL_LEVEL ${GV_LCORES_PLUS1})
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${CMAKE_BUILD_PARALLEL_LEVEL})

# Link libraries
set(PLATFORM_LINK_LIB "")
