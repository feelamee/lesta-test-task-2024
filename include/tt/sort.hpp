#pragma once

#include <tt/detail.hpp>

#include <algorithm>
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
template <std::unsigned_integral KeyType, typename Alloc = std::allocator<KeyType>>
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
        using std::views::pairwise, std::views::transform, std::views::reverse, std::ranges::size;

        std::vector<key_type, Alloc> count{ static_cast<std::size_t>(max) + 1, 0uz, alloc };

        for (auto const& i : r | transform(proj) | transform(key_fn)) count[i] += 1;

        for (auto [l, r] : count | pairwise) r += l;

        std::decay_t<Rng> out(size(r));
        for (auto const& el : r | reverse)
        {
            auto& i{ count[std::invoke(key_fn, std::invoke(proj, el))] };
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
    Radix sort

    time - O(r * k * n)
    space - O(1)

    where r is count of radices in radix_key_type
          k is the maximum value of key values
          n is count of elements in input sequence

    I call counting_sort r times.
    So time complexity is multiplication of r and time complexity of counting sort

    Radix sort is pretty similar to counting,
    but try to solve it problem - additional memory.
 */

// TODO: receive traits as value parameter instead of type?
//       this will allow to choose behaviour in runtime
template <typename T, typename KeyType>
concept radix_sort_traits = requires(std::size_t cur_radix, KeyType key) {
    typename T::radix_type;
    requires std::ranges::input_range<decltype(T::template radices<KeyType>)>;
    {
        std::invoke(std::invoke(T::template radix<KeyType>, cur_radix), key)
    } -> std::same_as<typename T::radix_type>;
};

struct default_radix_sort_traits
{
    using radix_type = std::uint8_t;

    template <typename KeyType>
    inline static constexpr auto radices{
        std::views::iota(0UL, detail::divceil(sizeof(KeyType), sizeof(radix_type)))
    };

    template <typename KeyType>
    inline static constexpr auto radix = [](std::size_t const cur_radix)
    {
        return [=](auto const key) -> radix_type
        { return (key >> (cur_radix * 8)) & static_cast<KeyType>(0xFF); };
    };
};
static_assert(radix_sort_traits<default_radix_sort_traits, std::size_t>);

template <typename Rng, typename KeyFn = std::identity, typename Proj = std::identity,
          typename Traits = default_radix_sort_traits>
    requires requires(KeyFn key_fn, Proj proj, std::ranges::range_value_t<Rng> v) {
        {
            std::invoke(key_fn, std::invoke(proj, v))
        } -> std::convertible_to<typename Traits::radix_type>;
    }
constexpr void
radix_sort(Rng&& r, KeyFn key_fn = {}, Proj proj = {})
{
    using std::views::iota, std::views::stride, std::ranges::size, std::ranges::move;

    using value_type = std::ranges::range_value_t<Rng>;
    using proj_result = std::invoke_result_t<Proj, value_type>;
    using key_fn_result = std::invoke_result_t<KeyFn, proj_result>;
    using key_type = std::decay_t<key_fn_result>;
    static_assert(radix_sort_traits<Traits, key_type>);
    using radix_type = typename Traits::radix_type;

    using allocator_type = std::pmr::polymorphic_allocator<radix_type>;
    auto constexpr max{ std::numeric_limits<radix_type>::max() };
    std::size_t constexpr max_as_size{ max };

    radix_type buf[max_as_size + 1];
    std::pmr::monotonic_buffer_resource res{ buf, max_as_size + 1,
                                             std::pmr::null_memory_resource() };
    allocator_type alloc{ &res };
    counting_sort_t<radix_type, allocator_type> sort{ alloc };

    for (auto const cur_radix : Traits::template radices<key_type>)
    {
        sort(
            r, max,
            [&key_fn, cur_radix](auto const& el)
            { return Traits::template radix<key_type>(cur_radix)(std::invoke(key_fn, el)); },
            proj);
        res.release();
    }
}

} // namespace tt
