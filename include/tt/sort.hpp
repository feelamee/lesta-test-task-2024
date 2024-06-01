#pragma once

#include <tt/detail.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <memory_resource>
#include <ranges>

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

    So, I decide to implement two quite similar sort algorithms:
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
// TODO: think about adding execution policy
template <typename Rng, typename Out, std::unsigned_integral KeyType, typename KeyFn = std::identity,
          typename Proj = std::identity, template <typename> typename Alloc = std::allocator>
    requires detail::composable<KeyFn, Proj, std::ranges::range_value_t<Rng>> &&
             std::constructible_from<KeyType, detail::compose_result_t<KeyFn, Proj, std::ranges::range_value_t<Rng>>> &&
             std::indirectly_writable<Out, std::ranges::range_value_t<Rng>>
constexpr void
counting_sort(Rng&& r, Out out, KeyType max, KeyFn key_fn = {}, Proj proj = {},
              Alloc<std::size_t> const& alloc = {})
{
    namespace vs = std::views;
    using allocator_type = Alloc<std::size_t>;

    std::vector<std::size_t, allocator_type> count{ static_cast<std::size_t>(max) + 1, 0uz, alloc };

    for (auto const& i : r | vs::transform(proj) | vs::transform(key_fn)) count[i] += 1;

    for (auto [l, r] : count | vs::pairwise) r += l;

    for (auto&& el : r | vs::reverse)
    {
        auto& i{ count[std::invoke(key_fn, std::invoke(proj, el))] };
        --i;
        out[i] = std::forward<decltype(el)>(el);
    }
}

template <typename Rng, typename Out, typename KeyFn = std::identity, typename Proj = std::identity,
          template <typename> typename Alloc = std::allocator>
    requires detail::composable<KeyFn, Proj, std::ranges::range_value_t<Rng>> &&
             std::indirectly_writable<Out, std::ranges::range_value_t<Rng>>
constexpr void
counting_sort(Rng&& r, Out out, KeyFn key_fn = {}, Proj proj = {}, Alloc<std::size_t> const& alloc = {})
{
    namespace rng = std::ranges;

    auto const composed = [&key_fn, &proj](auto const& el)
    { return std::invoke(key_fn, std::invoke(proj, el)); };

    auto const max{ rng::max_element(r, {}, composed) };
    if (max != rng::end(r))
    {
        counting_sort(std::forward<Rng>(r), std::move(out), std::move(*max), std::move(key_fn),
                      std::move(proj), alloc);
    }
}
/*
    Radix sort

    time - O(r * (k + n))
    space - O(1)

    where r is count of radices in radix_key_type
          k is the maximum value of key values
          n is count of elements in input sequence

    I call counting_sort r times.
    So time complexity is multiplication of r and time complexity of counting sort

    Radix sort is pretty similar to counting,
    but try to solve it problem - additional memory.
 */
template <typename R>
concept radix_traits = requires(R r, std::size_t n) {
    requires std::unsigned_integral<typename R::radix_type>;
    typename R::key_type;
    {
        r.radices()
    } -> std::ranges::range;
    {
        r.nth_radix_proj(n)(std::declval<typename R::key_type>())
    } -> std::same_as<typename R::radix_type>;
};

template <typename KeyType>
struct byte_radix_traits
{
    using radix_type = std::uint8_t;
    using key_type = KeyType;

    constexpr auto
    radices()
    {
        return std::views::iota(0UL, detail::divceil(sizeof(key_type), sizeof(radix_type)));
    };

    constexpr auto
    nth_radix_proj(std::size_t const cur_radix)
    {
        return [=](auto const key) -> radix_type
        { return (key >> (cur_radix * 8)) & static_cast<key_type>(0xFF); };
    };
};
static_assert(radix_traits<byte_radix_traits<std::size_t>>);

template <typename Rng, typename Out, typename KeyFn = std::identity, typename Proj = std::identity,
          radix_traits Traits = byte_radix_traits<detail::compose_result_t<KeyFn, Proj, std::ranges::range_value_t<Rng>>>>

    requires requires(KeyFn key_fn, Proj proj, Traits traits, std::ranges::range_value_t<Rng> v, std::size_t cur) {
        {
            traits.nth_radix_proj(cur)(std::invoke(key_fn, std::invoke(proj, v)))
        } -> std::same_as<typename Traits::radix_type>;

        requires std::indirectly_writable<Out, std::ranges::range_value_t<Rng>>;
        requires std::indirectly_swappable<std::ranges::iterator_t<Rng>, Out>;
    }
constexpr void
radix_sort(Rng&& r, Out out, KeyFn key_fn = {}, Proj proj = {}, Traits traits = {})
{
    namespace rng = std::ranges;
    using radix_type = typename Traits::radix_type;

    constexpr auto max{ std::numeric_limits<radix_type>::max() };
    constexpr auto buf_size{ static_cast<std::size_t>(max) + 1 };

    using buf_value_type = std::size_t;
    std::array<buf_value_type, buf_size> buf;
    std::pmr::monotonic_buffer_resource resource{ buf.data(), buf.size() * sizeof(buf_value_type),
                                                  std::pmr::null_memory_resource() };

    using allocator_type = std::pmr::polymorphic_allocator<buf_value_type>;
    allocator_type alloc{ &resource };

    for (auto const cur_radix : traits.radices())
    {
        counting_sort(
            r, out, max,
            [&key_fn, cur_radix, &traits](auto const& el)
            { return traits.nth_radix_proj(cur_radix)(std::invoke(key_fn, el)); },
            proj, alloc);

        rng::move(out, rng::next(out, rng::size(r)), rng::begin(r));
        resource.release();
    }
    rng::move(r, out);
}
} // namespace tt
