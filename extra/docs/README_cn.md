# Cppt Coroutine
[English](../..//README.md)

一个 C++ 有栈协程调度器

## 简介
此项目实现了一个 C++ 有栈协程调度器，主要特点如下：
- 提供了十分简单易用的接口，相比其它各种 C++ 调度器更易上手；
- 实现了高性能的 channel 功能，使协程间通信变得简单；
- 实现了易用的高性能 np_queue（Notify to polling queue），
  - 不需要忙等待或者繁杂的 callback 逻辑也能方便处理新来的数据；
  - 使得普通线程与协程之间的通信既方便又能避免各种同步问题；
- 必备的协程版 Mutex、Condition variable；
- 基于 `boost.context`，性能高，平台兼容性好；
- 支持 Work-Stealing，任务负载更均衡；
- 支持自动分离耗时任务。

## 依赖
此项目的依赖库由 `conan` 包管理器进行管理，请先安装 `conan`。在使用 cmake 配置项目的时候会自动使用 conan 安装依赖库。

**注意：**  
如果 `conan` 对编译器的配置不当，`conan install` 命令可能会运行失败。你可以在 `conan` 配置文件中检查相关配置。

配置示例：  
`~/.conan/profiles/default` 文件的 `[settings]` 部分，gcc 9，c++11 ABI：
```
compiler=gcc
compiler.version=9
compiler.libcxx=libstdc++11
```

## Build
Build 命令示例：
```shell
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
