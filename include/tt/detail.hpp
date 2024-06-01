#pragma once

#include <cassert>
#include <cstdlib>
#include <format>
#include <iostream>
#include <source_location>
#include <type_traits>

#if __cpp_lib_stacktrace >= 202011L
#include <stacktrace>
#endif

template <>
struct std::formatter<std::source_location>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(std::source_location const& obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}:{}:{} in '{}'", obj.file_name(), obj.line(),
                              obj.column(), obj.function_name());
    }
};

namespace tt::detail
{

// with thanks stolen from https://github.com/hsutter/cppfront/
template <typename T>
constexpr bool prefer_pass_by_value =
    sizeof(T) <= 2 * sizeof(void*) && std::is_trivially_copy_constructible_v<T>;

template <typename T>
    requires std::is_class_v<T> || std::is_union_v<T> || std::is_array_v<T> || std::is_function_v<T>
constexpr bool prefer_pass_by_value<T> = false;

template <typename T>
    requires(!std::is_void_v<T>)
using param = std::conditional_t<prefer_pass_by_value<T>, T const, T const&>;

[[noreturn]] inline void
unimplemented(std::source_location src = std::source_location::current())
{
    std::cerr << std::format("[unimplemented] {}\n", src);

#if __cpp_lib_stacktrace >= 202011L
    std::cerr << std::stacktrace::current() << std::endl;
#elif __cpp_lib_formatters >= 202302L
    std::cerr << std::format("{}\n", std::stacktrace::current());
#endif

    std::exit(EXIT_FAILURE);
}

template <std::unsigned_integral T>
bool
is_power_of_2(T const v)
{
    return v > 0 && !(v & (v - 1));
}

///! @pre l != 0 && r != 0
constexpr std::size_t
divceil(std::size_t const l, std::size_t const r)
{
    assert(l != 0 && r != 0);
    return 1 + ((l - 1) / r);
}

template <typename F1, typename F2, typename... Args>
concept composable =
    std::invocable<F2, Args...> && std::invocable<F1, std::invoke_result_t<F2, Args...>>;

template <typename F1, typename F2, typename... Args>
using compose_result_t = std::invoke_result_t<F1, std::invoke_result_t<F2, Args...>>;

} // namespace tt::detail
