#pragma once

#include <cstdlib>
#include <format>
#include <iostream>
#include <source_location>
#include <type_traits>

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
    std::exit(EXIT_FAILURE);
}

template <std::unsigned_integral T>
bool
is_power_of_2(T const v)
{
    return v > 0 && !(v & (v - 1));
}

template <typename F1, typename F2, typename Arg, typename Expected>
struct invoke_result_is_convertible_to
    : public std::is_convertible<std::invoke_result_t<F1, std::invoke_result_t<F2, Arg>>, Expected>
{
};

template <typename F1, typename F2, typename Arg, typename Expected>
inline constexpr bool invoke_result_is_convertible_to_v =
    invoke_result_is_convertible_to<F1, F2, Arg, Expected>::value;

} // namespace tt::detail
