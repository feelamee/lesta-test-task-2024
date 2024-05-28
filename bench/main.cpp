#include <benchmark/benchmark.h>

#include <tt/sort.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <random>

template <std::size_t size, std::uniform_random_bit_generator G = std::mt19937>
constexpr auto
rndseq(std::uint32_t seed = G::default_seed)
{
    G engine{ seed };
    std::array<typename G::result_type, size> ret = { 0 };
    std::ranges::generate(ret, engine);
    return ret;
}

void
radix_sort(benchmark::State& state)
{
    auto seq{ rndseq<100000>() };
    decltype(seq) res{ seq };

    for (auto _ : state) tt::radix_sort(seq, begin(res));

    assert(std::ranges::is_sorted(res));
}
BENCHMARK(radix_sort);

BENCHMARK_MAIN();
