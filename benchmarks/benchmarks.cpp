#include <benchmark/benchmark.h>
#include <numeric>
#include <vector>
#include "memory/arena_allocator.h"
#include "etl/pipeline.h"
#include "concurrency/lock_free_queue.h"

// ── Arena allocator vs std::allocator ───────────────────────────────

static void BM_StdAllocator(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v;
        for (int i = 0; i < state.range(0); ++i)
            v.push_back(i);
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_StdAllocator)->Range(64, 1 << 16);

static void BM_ArenaAllocator(benchmark::State& state) {
    for (auto _ : state) {
        memory::Arena arena(static_cast<std::size_t>(state.range(0)) * sizeof(int) * 2);
        memory::ArenaAllocator<int> alloc(arena);
        std::vector<int, memory::ArenaAllocator<int>> v(alloc);
        for (int i = 0; i < state.range(0); ++i)
            v.push_back(i);
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_ArenaAllocator)->Range(64, 1 << 16);

// ── ETL pipeline vs raw loop ────────────────────────────────────────

static void BM_RawLoop(benchmark::State& state) {
    std::vector<int> data(static_cast<std::size_t>(state.range(0)));
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        int sum = 0;
        for (auto x : data)
            if (x % 2 == 0) sum += x * x;
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_RawLoop)->Range(256, 1 << 16);

static void BM_EtlPipeline(benchmark::State& state) {
    std::vector<int> data(static_cast<std::size_t>(state.range(0)));
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        auto sum = etl::from(data)
            .filter([](int x) { return x % 2 == 0; })
            .map([](int x) { return x * x; })
            .reduce(0, std::plus<>{});
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_EtlPipeline)->Range(256, 1 << 16);

// ── SPSC queue throughput ───────────────────────────────────────────

static void BM_SpscThroughput(benchmark::State& state) {
    concurrency::SpscQueue<int, 16384> q;
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            q.push(i);
        for (int i = 0; i < state.range(0); ++i)
            benchmark::DoNotOptimize(q.pop());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SpscThroughput)->Range(256, 8192);
