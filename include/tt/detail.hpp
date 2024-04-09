#pragma once

#include <type_traits>

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

} // namespace tt::detail
