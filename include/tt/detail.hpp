#pragma once

#include <cstdlib>
#include <format>
#include <iostream>
#include <limits>
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
        return std::format_to(ctx.out(),
                              "{}:{}:{} in '{}'",
                              obj.file_name(),
                              obj.line(),
                              obj.column(),
                              obj.function_name());
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

} // namespace tt::detail
