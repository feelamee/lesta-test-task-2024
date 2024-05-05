#include <tt/lock_free_ringbuf.hpp>

#include "doctest.h"

#include <thread>
#include <vector>

TEST_SUITE("lock_free_ringbuif")
{
    TEST_CASE("empty/move ctor")
    {
        std::size_t capacity{ 2 };
        REQUIRE(tt::detail::is_power_of_2(capacity));

        tt::lock_free_ringbuf<int> buf{ capacity };
        REQUIRE_EQ(capacity, buf.capacity());
        REQUIRE(buf.empty());
        REQUIRE(!buf.full());

        tt::lock_free_ringbuf<int> buf2{ std::move(buf) };
        REQUIRE_EQ(capacity, buf2.capacity());
        REQUIRE(buf2.empty());
        REQUIRE(!buf2.full());
    }

    TEST_CASE("push_back/pop_front")
    {
        std::size_t const capacity1{ 1 };
        REQUIRE(tt::detail::is_power_of_2(capacity1));

        tt::lock_free_ringbuf<int> buf{ capacity1 };
        buf.push_back(42);
        REQUIRE(!buf.empty());
        REQUIRE_EQ(1, buf.size());
        REQUIRE(buf.full());

        auto v{ buf.pop_front() };
        REQUIRE(v.has_value());
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());

        v = buf.pop_front();
        REQUIRE(!v.has_value());
    }
    TEST_CASE("emplace_back/pop_front")
    {
        std::size_t const capacity1{ 1 };
        REQUIRE(tt::detail::is_power_of_2(capacity1));

        tt::lock_free_ringbuf<int> buf{ capacity1 };
        buf.emplace_back(42);
        REQUIRE(!buf.empty());
        REQUIRE_EQ(1, buf.size());
        REQUIRE(buf.full());

        auto v{ buf.pop_front() };
        REQUIRE(v.has_value());
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());

        v = buf.pop_front();
        REQUIRE(!v.has_value());
    }

    TEST_CASE("emplace_back/pop_front several times")
    {
        std::size_t const capacity1{ 4 };
        REQUIRE(tt::detail::is_power_of_2(capacity1));

        tt::lock_free_ringbuf<int> buf{ capacity1 };
        buf.emplace_back(42);
        buf.emplace_back(43);
        buf.emplace_back(44);
        REQUIRE(!buf.empty());
        REQUIRE_EQ(3, buf.size());
        REQUIRE(!buf.full());

        {
            auto v{ buf.pop_front() };
            REQUIRE(v.has_value());
            REQUIRE_EQ(42, *v);
            REQUIRE(!buf.empty());
            REQUIRE_EQ(2, buf.size());
        }

        {
            auto v{ buf.pop_front() };
            REQUIRE(v.has_value());
            REQUIRE_EQ(43, *v);
            REQUIRE(!buf.empty());
            REQUIRE_EQ(1, buf.size());
        }

        {
            auto v{ buf.pop_front() };
            REQUIRE(v.has_value());
            REQUIRE_EQ(44, *v);
            REQUIRE(buf.empty());
            REQUIRE_EQ(0, buf.size());
        }
    }

    TEST_CASE("emplace_back/pop_front with overwrite")
    {
        std::size_t const capacity1{ 1 };
        REQUIRE(tt::detail::is_power_of_2(capacity1));

        tt::lock_free_ringbuf<int> buf{ capacity1 };
        buf.emplace_back(42);
        buf.emplace_back(43);
        buf.emplace_back(44);
        REQUIRE(!buf.empty());
        REQUIRE_EQ(1, buf.size());
        REQUIRE(buf.full());

        auto v{ buf.pop_front() };
        REQUIRE(v.has_value());
        REQUIRE_EQ(44, *v);
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());
    }

    template <typename T>
    void produce(tt::lock_free_ringbuf<int> & buf, std::atomic<int> & tasks_count)
    {
        for (;;)
        {
            int n{ tasks_count.load() };
            do
            {
                if (n < 0) return;
                std::this_thread::yield();
            } while (!tasks_count.compare_exchange_weak(n, n - 1));

            buf.push_back(n);
        }
    }

    template <typename T>
    void consume(tt::lock_free_ringbuf<T> & buf, std::atomic<int> & tasks_count,
                 std::atomic<std::size_t> & consumed_count)
    {
        for (;;)
        {
            if (tasks_count.load() < 0 && buf.empty()) break;
            auto const res = buf.pop_front();
            if (res) consumed_count.fetch_add(1);
        }
    };

    TEST_CASE("produce/consume")
    {
        std::size_t const capacity{ 1 };
        REQUIRE(tt::detail::is_power_of_2(capacity));
        tt::lock_free_ringbuf<int> buf{ capacity };

        int const tasks_count{ 1000 };

        std::atomic<std::size_t> consumed_count{ 0 };
        std::atomic<int> counter{ tasks_count };

        std::uint32_t const producers_count{ 1 };
        std::uint32_t const consumers_count{ 1 };
        std::vector<std::thread> threads;
        threads.resize(producers_count + consumers_count);

        // clang-format off
        auto last =
        std::generate_n(begin(threads), producers_count,
                        [&] { return std::thread(produce<int>,
                                                 std::ref(buf),
                                                 std::ref(counter)); });
        std::generate_n(last, consumers_count,
                        [&] { return std::thread(consume<int>,
                                                 std::ref(buf),
                                                 std::ref(counter),
                                                 std::ref(consumed_count)); });
        // clang-format on

        std::ranges::for_each(threads, [](auto& t) { t.join(); });

        REQUIRE_EQ(counter.load(), -1);
        REQUIRE_EQ(consumed_count.load(), tasks_count + 1);
    }
}
