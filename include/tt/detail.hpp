#pragma once

#include <type_traits>

namespace tt::detail
{

template <typename T>
using param_t = std::conditional_t<(sizeof(T) > sizeof(void*)), T&, T>;

} // namespace tt::detail
