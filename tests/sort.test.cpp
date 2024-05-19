#include <tt/sort.hpp>

#include "doctest.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>
#include <sstream>
#include <vector>

namespace doctest
{

template <typename T>
struct StringMaker<std::vector<T>>
{
    static String
    convert(const std::vector<T>& seq)
    {
        std::stringstream out;
        out << '[';
        for (auto const& el : seq) out << std::format(" {} ", el);
        out << ']' << std::endl;
        return doctest::String(out.str().c_str());
    }
};

} // namespace doctest

template <typename... Args>
void
println(std::format_string<Args...> fmt, Args&&... args)
{
    std::cout << std::format(fmt, std::forward<Args>(args)...) << std::endl;
};

TEST_SUITE("sort")
{
    TEST_CASE("counting sort on empty array")
    {
        using std::views::all;

        std::vector<int> const ar;
        REQUIRE(ar.empty());
        std::vector<int> res{ ar };
        tt::counting_sort(res, 0);
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("counting sort with projection")
    {
        using std::views::all;
        struct num
        {
            num() = default;
            num(int p_n)
                : n{ p_n }
            {
            }
            int n{ 0 };
        };
        std::vector<num> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        auto const max{ *std::ranges::max_element(ar, {}, &num::n) };
        std::vector<num> res{ ar };
        tt::counting_sort(res, max.n, {}, &num::n);
        REQUIRE(std::ranges::is_sorted(res, {}, &num::n));
    }

    TEST_CASE("counting sort")
    {
        using std::views::all;

        std::vector<int> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        auto const max{ *std::ranges::max_element(ar) };
        std::vector<int> res{ ar };
        tt::counting_sort(res, max);
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("counting sort without max parameter")
    {
        using std::views::all;

        std::vector<int> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        std::vector<int> res{ ar };
        tt::counting_sort(res);
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("radix sort")
    {
        using std::views::all;
        auto const sorted = [](auto input)
        {
            std::ranges::sort(input);
            return input;
        };

        SUBCASE("1")
        {
            std::vector<int> const ar{ 3, 5, 1, 8, 10, 0, 14 };
            std::vector<int> res{ ar };
            tt::radix_sort(res);
            CHECK_EQ(res, sorted(ar));
        }
        SUBCASE("2")
        {
            std::vector<int> const ar{ 8, 7, 6, 5, 4, 3, 2, 1 };
            std::vector<int> res{ ar };
            tt::radix_sort(res);
            CHECK_EQ(res, sorted(ar));
        }
        SUBCASE("3")
        {
            std::vector<int> const ar{ 508, 507, 606, 505 };
            std::vector<int> res{ ar };
            tt::radix_sort(res);
            CHECK_EQ(res, sorted(ar));
        }
        SUBCASE("4")
        {
            std::vector<int> ar(10);
            std::ranges::generate(ar, [] { return std::rand() % 1000; });
            std::vector<int> res{ ar };
            tt::radix_sort(res);
            CHECK_EQ(res, sorted(ar));
        }
        SUBCASE("5")
        {
            std::vector<int> ar{ 5321, 4, 41, 510, 140, 0, 43, 3, 31231 };
            std::vector<int> res{ ar };
            tt::radix_sort(res);
            CHECK_EQ(res, sorted(ar));
        }
    }
}
