#include <doctest/doctest.h>

#include <tt/sort.hpp>

#include <algorithm>
#include <format>
#include <iostream>
#include <memory_resource>
#include <random>
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

        std::vector<uint> const ar;
        REQUIRE(ar.empty());
        std::vector<uint> res(ar.size());
        tt::counting_sort(ar, begin(res), 0);
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("counting sort with projection")
    {
        using std::views::all;
        struct num
        {
            num() = default;
            num(uint p_n)
                : n{ p_n }
            {
            }
            uint n{ 0 };
        };
        std::vector<num> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        auto const max{ *std::ranges::max_element(ar, {}, &num::n) };
        std::vector<num> res(ar.size());
        tt::counting_sort(ar, begin(res), max.n, {}, &num::n);
        REQUIRE(std::ranges::is_sorted(res, {}, &num::n));
    }

    TEST_CASE("counting sort")
    {
        using std::views::all;

        std::vector<uint> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        auto const max{ *std::ranges::max_element(ar) };
        std::vector<uint> res(ar.size());
        tt::counting_sort(ar, begin(res), max);
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("counting sort without max parameter")
    {
        using std::views::all;

        std::vector<uint> const ar{ 3, 5, 1, 8, 10, 0, 14 };
        std::vector<uint> res(ar.size());
        tt::counting_sort(ar, begin(res));
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("counting sort with big array")
    {
        using std::views::all;

        std::vector<uint> ar(10000);
        std::ranges::generate(ar, [] { return std::rand() % 1000000; });

        std::vector<uint> res(ar.size());
        tt::counting_sort(ar, begin(res));
        REQUIRE(std::ranges::is_sorted(res));
    }

    TEST_CASE("custom counting sort with big array")
    {
        using std::views::all;

        std::vector<uint> ar(10000);
        std::ranges::generate(ar, [] { return std::rand() % 1000000; });

        std::vector<uint> res(ar.size());

        tt::counting_sort_t<uint, std::pmr::polymorphic_allocator<uint>> sort;
        sort(ar, begin(res));
        REQUIRE(std::ranges::is_sorted(res));
    }

    // TODO: see compile errors of this test and concrete requires of input type for sort
    // TEST_CASE("counting sort shuffled view")
    // {
    //     using std::views::iota, std::ranges::shuffle;

    //     auto ar = iota(0, 100);
    //     tt::counting_sort(ar);
    //     REQUIRE(std::ranges::is_sorted(ar));
    // }

    TEST_CASE("radix sort")
    {
        using std::views::all;
        auto const sorted = [](auto input)
        {
            std::ranges::sort(input);
            return input;
        };

        std::vector<int> ar;

        SUBCASE("random1") { ar = { 3, 5, 1, 8, 10, 0, 14 }; }
        SUBCASE("random2") { ar = { 8, 7, 6, 5, 4, 3, 2, 1 }; }
        SUBCASE("2-3th radix") { ar = { 508, 507, 606, 505 }; }
        SUBCASE("random")
        {
            ar.resize(10000);
            std::ranges::generate(ar, [] { return std::rand() % 10000; });
        }
        SUBCASE("different radices") { ar = { 5321, 4, 41, 510, 140, 0, 43, 3, 31231 }; }

        std::vector<int> res(ar.size());
        tt::radix_sort(std::vector{ ar }, begin(res));
        CHECK_EQ(res, sorted(ar));
    }
}
