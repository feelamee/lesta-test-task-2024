#include <benchmark/benchmark.h>

#include <tt/sort.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <random>

template <std::uniform_random_bit_generator G = std::mt19937>
constexpr auto
rndseq(std::size_t size, std::uint32_t seed = G::default_seed)
{
    G engine{ seed };
    std::vector<typename G::result_type> ret(size);
    std::ranges::generate(ret, engine);
    return ret;
}

void
radix_sort(benchmark::State& state)
{
    auto seq{ rndseq(state.range(0)) };
    decltype(seq) res{ seq };

    for (auto _ : state) tt::radix_sort(seq, begin(res));

    assert(std::ranges::is_sorted(res));
    state.SetItemsProcessed(size(res));
    state.counters["array_size"] = size(res);
}
BENCHMARK(radix_sort)->RangeMultiplier(2)->Range(0, 1000000);

void
std_sort(benchmark::State& state)
{
    auto seq{ rndseq(state.range(0)) };
    decltype(seq) res{ seq };

    for (auto _ : state) std::ranges::sort(seq);

    assert(std::ranges::is_sorted(seq));
    state.SetItemsProcessed(size(res));
    state.counters["array_size"] = size(res);
}
BENCHMARK(std_sort)->RangeMultiplier(2)->Range(0, 1000000);

void
counting_sort(benchmark::State& state)
{
    auto seqview{ rndseq(state.range(0)) |
                  std::views::transform([&](auto el) { return el % 100000; }) };

    std::vector seq(begin(seqview), end(seqview));
    std::vector res(begin(seq), end(seq));

    for (auto _ : state) tt::counting_sort(seq, begin(res));

    assert(std::ranges::is_sorted(res));
    state.SetItemsProcessed(size(res));
    state.counters["array_size"] = size(res);
}
BENCHMARK(counting_sort)->RangeMultiplier(2)->Range(0, 1000000);

BENCHMARK_MAIN();
