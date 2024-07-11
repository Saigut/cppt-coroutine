
set(V_program_name test_http_client)
add_executable(${V_program_name} exec_main.cpp)
set(V_dep_libs ${CONAN_LIBS})
list(FILTER V_dep_libs EXCLUDE REGEX "boost")
target_link_libraries(${V_program_name}
    cppt_coroutine
    ${GV_gen_grpc_cpp_lib_bname}
    ${V_dep_libs})
target_link_boost_libraries(${V_program_name} boost_thread boost_context boost_atomic boost_chrono boost_date_time boost_program_options)
target_link_libraries(${V_program_name} ${PLATFORM_LINK_LIB})
