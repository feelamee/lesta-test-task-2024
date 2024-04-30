#pragma once

#include <tt/detail.hpp>

#include <cassert>
#include <functional>
#include <limits>
#include <memory>
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

template <typename Alloc = std::allocator<std::byte>>
struct counting_sort_t
{
    constexpr counting_sort_t(Alloc const& p_alloc = Alloc{})
        : alloc{ p_alloc }
    {
    }

    using key_type = std::size_t;

    ///! @pre max < std::numeric_limits<key_type>::max()
    template <typename Rng, typename KeyFn = std::identity, typename Proj = std::identity>
        requires std::ranges::constant_range<Rng> && std::ranges::random_access_range<Rng>
    constexpr auto
    operator()(Rng&& r, key_type max, KeyFn key_fn = {}, Proj proj = {}) const
    {
        assert(max < std::numeric_limits<key_type>::max());

        using value_type = std::ranges::range_value_t<Rng>;
        {
            using key_fn_result = std::invoke_result_t<KeyFn, value_type>;
            using proj_result = std::invoke_result_t<Proj, key_fn_result>;
            static_assert(std::convertible_to<proj_result, key_type>);
        }

        using value_type_alloc = std::allocator_traits<Alloc>::template rebind_alloc<value_type>;

        using counter_type = std::make_signed_t<key_type>;
        using counter_alloc = std::allocator_traits<Alloc>::template rebind_alloc<counter_type>;

        using std::ranges::size, std::ranges::owning_view;
        using std::views::pairwise, std::views::transform, std::views::reverse;

        std::vector<counter_type, counter_alloc> count{ max + 1, 0uz, alloc };
        std::vector<value_type, value_type_alloc> res{ size(r), value_type{}, alloc };

        for (auto const& i : r | transform(proj) | transform(key_fn)) count[i] += 1;

        for (auto [l, r] : count | pairwise) r += l;

        for (auto const& el : r | reverse | transform(proj))
        {
            auto& i{ count[key_fn(el)] };
            --i;
            res[i] = el;
        }

        return owning_view(std::move(res));
    }

private:
    Alloc alloc;
};

static constexpr counting_sort_t counting_sort;

} // namespace tt
