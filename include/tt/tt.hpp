#pragma once

#include <concepts>

namespace tt
{

template <std::integral T>
bool
iseven(T const n)
{
    return ~n & T{ 1 };
}

} // namespace tt
