include_guard()

include(ExternalProject)
include(common_var)

function(external_lib_setup baseDir)
    # Parallel argument of build
    set(PARALLEL_ARG "")
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
        if(GV_LCORES GREATER 0)
            set(PARALLEL_ARG "-j ${GV_LCORES_PLUS1}")
        endif()
    endif()

    set(BOOST_BS_SUFFIX "")
    if(OS_WINDOWS)
        set(BOOST_BS_SUFFIX ".bat")
    else()
        set(BOOST_BS_SUFFIX ".sh")
    endif()

    # setup package URL
    set(V_JSON_C_URL ${baseDir}/package/json-c.zip)
    set(V_BOOST_URL ${baseDir}/package/boost_1_79_0.zip)
    set(V_GRPC_URL ${baseDir}/package/grpc-1.34.0_with_deps.zip)
    if(NOT EXISTS "${V_JSON_C_URL}")
        set(V_JSON_C_URL https://github.com/json-c/json-c/archive/master.zip)
    endif()
    if(NOT EXISTS "${V_BOOST_URL}")
        set(V_BOOST_URL https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.zip)
    endif()
    if(NOT EXISTS "${V_GRPC_URL}")
        set(V_GRPC_URL https://github.com/Saigut/grpc/releases/download/v1.34.0/grpc-1.34.0_with_deps.zip)
    endif()

    # json-c
#    ExternalProject_Add(ep_json-c
#        URL ${V_JSON_C_URL}
#        DOWNLOAD_NAME json-c.zip
#        CMAKE_ARGS
#            -DBUILD_SHARED_LIBS=OFF
#            -DCMAKE_INSTALL_PREFIX:PATH=${baseDir}/external/${CMAKE_CXX_COMPILER_ID}
#            -DBUILD_TESTING=OFF
#            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#        BUILD_COMMAND ${CMAKE_COMMAND} --build . ${PARALLEL_ARG})

    # boost_1_79_0
#    ExternalProject_Add(ep_boost
#        URL ${V_BOOST_URL}
#        CONFIGURE_COMMAND ./bootstrap${BOOST_BS_SUFFIX}
#        BUILD_IN_SOURCE true
#        BUILD_COMMAND ""
#        INSTALL_COMMAND ./b2 --layout=system --prefix=${baseDir}/external/${CMAKE_CXX_COMPILER_ID} ${PARALLEL_ARG} address-model=64 architecture=x86 variant=release link=static install)

    # grpc-1.34.0
    # set openssl path
    string(LENGTH "${OPENSSL_ROOT_DIR}" StrLen)
    set(V_grpc_cmake_ssl_param "")
    if (NOT ${StrLen} EQUAL 0)
        set(V_grpc_cmake_ssl_param "-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
    endif()

#    ExternalProject_Add(ep_grpc
##        git clone -b v1.34.0 https://github.com/grpc/grpc
#        URL ${V_GRPC_URL}
#        DOWNLOAD_NAME grpc-1.34.0_with_deps.zip
##        UPDATE_COMMAND git submodule update --init
#        CMAKE_ARGS
#            -DCMAKE_INSTALL_PREFIX:PATH=${baseDir}/external/${CMAKE_CXX_COMPILER_ID}
#            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#            -DgRPC_SSL_PROVIDER=package
#            ${V_grpc_cmake_ssl_param}
#        BUILD_COMMAND ${CMAKE_COMMAND} --build . ${PARALLEL_ARG})
endfunction()
