#pragma once

#include <tt/detail.hpp>

#include <algorithm>
#include <concepts>
#include <functional>
#include <limits>
#include <memory>
#include <memory_resource>
#include <ranges>
#include <type_traits>

namespace tt
{

/*
    First of all, I need say, that typical choice of sorting algorithm is quick
   sort. There a good article about it - https://en.wikipedia.org/wiki/Quicksort

    But I will not implement it here for several reasons:
      - it shows a good asymptotics independency of data type,
        but task is more specialized - it require sort for 'integral' data type,
        so here we can do some interesting optimizations

      - this is just boring, anyone know about quick sort))

    So, I decide to implement three quite similar sort algorithms:
      - counting
      - radix
      - bucket

    Below, I will mediate about [dis]advantages of each.
    The most interesting awaits you in the end - benchmarks
*/

/*
    Counting sort

    time - O(n + k)
    memory - O(n + k)

    where n is number of elements in input sequence
          k is the maximum value of key values

    This is not comaprable, not in-place, stable sort
    So, it have good asymptotic complexity, but..
    if k much bigger n, it will use a lot of memory for nothing
*/
template <std::unsigned_integral KeyType, typename Alloc = std::allocator<std::byte>>
struct counting_sort_t
{
public:
    constexpr counting_sort_t(Alloc const& p_alloc = Alloc{})
        : alloc{ p_alloc }
    {
    }

    using key_type = KeyType;

    // TODO: think about adding execution policy
    template <typename Rng, typename KeyFn = std::identity, typename Proj = std::identity>
        requires requires(KeyFn key_fn, Proj proj, std::ranges::range_value_t<Rng> v) {
            {
                std::invoke(key_fn, std::invoke(proj, v))
            } -> std::convertible_to<key_type>;
        }
    constexpr void
    operator()(Rng&& r, key_type max, KeyFn key_fn = {}, Proj proj = {}) const
    {
        using counter_alloc = std::allocator_traits<Alloc>::template rebind_alloc<key_type>;
        using std::views::pairwise, std::views::transform, std::views::reverse;

        std::vector<key_type, counter_alloc> count{ static_cast<std::size_t>(max) + 1, 0uz, alloc };

        for (auto const& i : r | transform(proj) | transform(key_fn)) count[i] += 1;

        for (auto [l, r] : count | pairwise) r += l;

        std::decay_t<Rng> out(r.size());
        for (auto const& el : r | reverse | transform(proj))
        {
            auto& i{ count[key_fn(el)] };
            --i;
            out[i] = std::move(el);
        }
        std::ranges::move(out, begin(r));
    }

    template <typename Rng, typename KeyFn = std::identity, typename Proj = std::identity>
        requires requires(KeyFn key_fn, Proj proj, std::ranges::range_value_t<Rng> v) {
            {
                std::invoke(key_fn, std::invoke(proj, v))
            } -> std::convertible_to<key_type>;
        }
    constexpr void
    operator()(Rng&& r, KeyFn key_fn = {}, Proj proj = {}) const
    {
        auto const composed = [&key_fn, &proj](auto const& el) { return key_fn(proj(el)); };
        auto const max{ std::ranges::max_element(r, {}, composed) };
        if (max != end(r))
        {
            (*this)(std::forward<Rng>(r), std::move(*max), std::move(key_fn), std::move(proj));
        }
    }

private:
    Alloc alloc;
};
inline constexpr counting_sort_t<std::size_t> counting_sort{};

/*
 */

template <typename Alloc = std::allocator<std::byte>>
struct radix_sort_t
{
public:
    constexpr radix_sort_t(Alloc const& p_alloc = Alloc{})
        : alloc{ p_alloc }
    {
    }

    // TODO: think about adding execution policy

public:
    using radix_key_type = std::uint8_t;

    template <typename Rng, typename KeyFn = std::identity, typename Proj = std::identity>
        requires requires(KeyFn key_fn, Proj proj, std::ranges::range_value_t<Rng> v) {
            {
                std::invoke(key_fn, std::invoke(proj, v))
            } -> std::convertible_to<radix_key_type>;
        }
    constexpr auto
    operator()(Rng&& r, KeyFn key_fn = {}, Proj proj = {}) const
    {
        using std::views::iota, std::views::stride, std::ranges::size, std::ranges::move;

        using value_type = std::ranges::range_value_t<Rng>;
        using proj_result = std::invoke_result_t<Proj, value_type>;
        using key_fn_result = std::invoke_result_t<KeyFn, proj_result>;
        using key_type = std::decay_t<key_fn_result>;

        using allocator_type = std::pmr::polymorphic_allocator<radix_key_type>;
        auto const max{ std::numeric_limits<radix_key_type>::max() };

        radix_key_type buf[static_cast<std::size_t>(max) + 1];
        std::pmr::monotonic_buffer_resource res{ buf, max + 1, std::pmr::null_memory_resource() };
        allocator_type alloc{ &res };
        counting_sort_t<radix_key_type, allocator_type> sort{ alloc };

        std::decay_t<Rng> out(size(r));
        auto const digits{ std::numeric_limits<key_type>::digits };
        for (auto const radix : iota(0, digits) | stride(8))
        {
            sort(
                r, max,
                [&key_fn, radix](auto const& el) -> radix_key_type
                { return (key_fn(el) >> radix) & static_cast<key_type>(0xFF); },
                proj);
            res.release();
        }
    }

private:
    Alloc alloc;
};
inline constexpr radix_sort_t radix_sort{};

} // namespace tt
