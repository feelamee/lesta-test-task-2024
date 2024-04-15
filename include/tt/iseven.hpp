#pragma once

#include <concepts>

namespace tt
{

/*
    the diff between my(1) impl and your(2):
    ```
       bool isEven(int value){return value%2==0;}
    ```
    is that 1 use feature of hardware - binary representaion,
    it will not work on trinary processor, haha)
    and 2 use mathematical definition of even numbers

    Theoreticaly, 1 will be faster.
    But, in practice, compiler can optimize 2

    Althrough, game programmers live in debug build most of time,
    that's why this is important for game to be playable in debug.
*/
template <std::integral T>
bool
iseven(T const n)
{
    return ~n & T{ 1 };
}

} // namespace tt
