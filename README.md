# Cppt Coroutine
A C++ stackful coroutine scheduler

## Dependency
1. Install conan package manager
2. Adjust installation location of conan packages
    ```shell
    # show
    conan config get storage.path
    # change
    conan config set storage.path=<your path>
    ```

## Build
```shell
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
**Caution:** `conan install` maybe failed when `cmake ..` running, causing by compiler settings. Then you can refer to below settings for conan config file (eg: `~/.conan/profiles/default`, `[settings]` section, with gcc 9 and using c++11 ABI)
   ```
   compiler=gcc
   compiler.version=9
   compiler.libcxx=libstdc++11
   ```
