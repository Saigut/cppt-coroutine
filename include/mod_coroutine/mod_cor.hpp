#ifndef CPP_TOOLKIT_MOD_COR_HPP
#define CPP_TOOLKIT_MOD_COR_HPP

#include <boost/asio/io_context.hpp>
#include "mod_cor_impl.hpp"

namespace cppt {
    // cppt:
    // cor_sp
    // cor_sp->join()
    // cor_create()
    // cor_yield()
    // cor_run()

    using cor_sp = cppt_impl::cor_sp;

    template<typename Func, typename... Args>
    cor_sp cor_create_impl(Func&& func, Args&&... args) {
        auto params = std::make_tuple(std::forward<Args>(args)...);
        auto user_co = [=](){
            cppt_impl::co_call_with_variadic_arg(func, params);
        };
        return cppt_impl::cppt_co_create0(std::move(user_co));
    }

//    template<typename Func, typename Obj, typename... Args>
//    cor_sp cor_create_mf(Func&& func, Obj&& obj, Args&&... args) {
//        return cor_create_impl(std::mem_fn(func), std::forward<Obj>(obj), std::forward<Args>(args)...);
//    }

    template<typename Func, typename... Args>
    cor_sp cor_create(Func&& func, Args&&... args) {
        if constexpr (std::is_member_function_pointer_v<std::remove_reference_t<Func>>) {
//            return cor_create_mf(func, std::forward<Args>(args)...);
            return cor_create_impl(std::mem_fn(func), std::forward<Args>(args)...);
        } else {
            return cor_create_impl(func, std::forward<Args>(args)...);
        }
    }

    template<typename Func, typename... Args>
    cor_sp cor_create_impl(Func& func, Args&&... args) {
        auto params = std::make_tuple(std::forward<Args>(args)...);
        auto user_co = [=](){
            cppt_impl::co_call_with_variadic_arg(func, params);
        };
        return cppt_impl::cppt_co_create0(std::move(user_co));
    }

    template<typename Func, typename... Args>
    cor_sp cor_create(Func& func, Args&&... args) {
        if constexpr (std::is_member_function_pointer<std::remove_reference_t<Func>>::value) {
            return cor_create_impl(std::mem_fn(func), std::forward<Args>(args)...);
        } else {
            return cor_create_impl(func, std::forward<Args>(args)...);
        }
    }

    // ret: 0, ok; -1 coroutine error
    int cor_yield();

    // ret: 0, ok; -1 coroutine error
    int cor_yield(
            const std::function<void(std::function<void(int yield_ret_val)>&& resume_f)>& wrapped_extern_func);

    // ret: 0, ok; 1 timeout
    int cor_yield(
            const std::function<void(std::function<void(int yield_ret_val)>&& resume_f)>& wrapped_extern_func,
            unsigned int timeout_ms,
            const std::function<void()>& f_cancel_operation);

    void cor_run();

    boost::asio::io_context& cor_get_io_context();
}

#endif //CPP_TOOLKIT_MOD_COR_HPP
