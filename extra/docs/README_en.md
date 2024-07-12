# Cppt Coroutine
[中文](../../README.md)

A C++ stackful coroutine scheduler

## Introduction
This project implements a C++ stackful coroutine scheduler with the following main features:
- Provides a very simple and user-friendly interface, making it easier to use compared to other various C++ schedulers;
- Implements a high-performance channel feature, simplifying communication between coroutines;
- Implements an easy-to-use and high-performance np_queue (Notify to polling queue),
   - Allows handling new data conveniently without busy waiting or complex callback logic;
   - Facilitates communication between regular threads and coroutines, avoiding various synchronization issues;
- Includes essential coroutine versions of Mutex and Condition variable;
- Based on `boost.context`, offering high performance and good platform compatibility;
- Supports work-stealing for more balanced task load;
- Supports automatic separation of time-consuming tasks.

## Dependencies
The dependencies for this project are managed by the `conan` package manager. Please install `conan` first. When configuring the project with cmake, conan will automatically install the dependencies.

**Note:**  
If `conan` is not properly configured for the compiler, the `conan install` command may fail. You can check the relevant configuration in the `conan` configuration file.

Configuration example:  
In the `[settings]` section of the `~/.conan/profiles/default` file, for gcc 9 with c++11 ABI:
```
compiler=gcc
compiler.version=9
compiler.libcxx=libstdc++11
```

## Build
Build command example:
```shell
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
