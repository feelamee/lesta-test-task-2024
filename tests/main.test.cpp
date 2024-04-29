#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <tt/detail.hpp>
#include <tt/iseven.hpp>
#include <tt/lock_free_ringbuf.hpp>
#include <tt/ringbuf.hpp>

#include <thread>

TEST_CASE("iseven")
{
    REQUIRE_EQ(true, tt::iseven(0));

    REQUIRE_EQ(true, tt::iseven(2));
    REQUIRE_EQ(true, tt::iseven(-2));

    REQUIRE_EQ(false, tt::iseven(1));
    REQUIRE_EQ(false, tt::iseven(-1));
    REQUIRE_EQ(false, tt::iseven(3));
}

TEST_CASE("ringbuf::empty/size/full")
{
    tt::ringbuf<int> buf{ 0 };
    REQUIRE(buf.empty());
    REQUIRE_EQ(0, buf.size());
    REQUIRE(buf.full());
}

TEST_CASE("ringbuf::operator==")
{
    tt::ringbuf<int> buf1{ 0 };
    tt::ringbuf<int> buf2{ 1 };
    REQUIRE_EQ(buf1, buf2);
}

TEST_CASE("ringbuf::max_size")
{
    using value_type = int;
    using allocator_type = std::allocator<value_type>;
    using allocator_traits = std::allocator_traits<allocator_type>;

    {
        allocator_type alloc;
        tt::ringbuf<value_type, allocator_type> buf(0, alloc);
        REQUIRE_EQ(buf.max_size(), allocator_traits::max_size(alloc));
    }

    {
        struct custom_allocator : public std::allocator<value_type>
        {
            int id;
        };
        custom_allocator alloc1{ .id = 42 };
        custom_allocator alloc2{ .id = 69 };
        tt::ringbuf<value_type, custom_allocator> buf(42, alloc1);
        REQUIRE_EQ(alloc1.id, buf.get_allocator().id);
        REQUIRE_NE(alloc2.id, buf.get_allocator().id);
    }
}

TEST_CASE("ringubf::emplace_back")
{
    tt::ringbuf<int> buf{ 1 };
    REQUIRE_EQ(buf.begin(), buf.end());

    int const val{ 42 };
    buf.emplace_back(val);
    REQUIRE_EQ(buf.size(), 1);
    REQUIRE_EQ(buf.begin() + 1, buf.end());
    REQUIRE_EQ(*buf.begin(), val);
}

TEST_CASE("ringbuf::emplace_back with overwrite")
{
    tt::ringbuf<int> buf1{ 1 };
    REQUIRE(buf1.empty());
    buf1.emplace_back(5);
    REQUIRE(buf1.full());
    buf1.clear();
    REQUIRE(buf1.empty());
    {
        tt::ringbuf<int> buf2{ buf1 };
        REQUIRE_EQ(buf2.capacity(), 1);

        buf2.emplace_back(5);
        REQUIRE_NE(buf1, buf2);
    }
    {
        auto buf2{ buf1 };
        buf2.emplace_back(5);
        REQUIRE_EQ(*buf2.begin(), 5);
        auto buf3{ buf2 };
        buf3.emplace_back(6);
        REQUIRE_EQ(*buf3.begin(), 6);

        REQUIRE_EQ(buf3.size(), buf2.size());
    }
    {
        tt::ringbuf buf2{ buf1 };
        buf2.emplace_back(5);

        tt::ringbuf<int> buf3{ buf2.capacity() };
        REQUIRE_NE(buf2, buf3);
        buf3 = buf2;
        REQUIRE_EQ(buf2, buf3);
    }
}

TEST_CASE("ringbuf::emplace_back/pop_back")
{
    tt::ringbuf<int> buf{ 2 };
    buf.emplace_back(42);
    auto const v{ buf.pop_back() };
    REQUIRE(v.has_value());
    REQUIRE_EQ(42, *v);
    REQUIRE(buf.empty());
}

TEST_CASE("ringbuf::pop_back on empty")
{
    tt::ringbuf<int> buf{ 2 };
    auto const v{ buf.pop_back() };
    REQUIRE(!v.has_value());
    REQUIRE_EQ(std::optional<int>(), v);
}

TEST_CASE("ringbuf::emplace_back/pop_front")
{
    tt::ringbuf<int> buf{ 2 };
    buf.emplace_back(42);
    auto const v{ buf.pop_front() };
    REQUIRE(v.has_value());
    REQUIRE_EQ(42, *v);
    REQUIRE(buf.empty());
}

TEST_CASE("ringbuf::append_range")
{
    std::size_t const capacity1{ 5 };
    tt::ringbuf<int> buf1{ capacity1 };

    using std::views::all;
    buf1.append_range(std::array{ 1, 2, 3, 4, 5, 6 } | all);
    std::array expected{ 2, 3, 4, 5, 6 };
    auto i{ buf1.begin() };
    auto j{ expected.begin() };
    for (; i != buf1.end() && j != expected.end(); ++i, ++j) REQUIRE_EQ(*i, *j);
}

TEST_CASE("ringbuf::swap")
{
    std::size_t const capacity1{ 5 };
    tt::ringbuf<int> buf1{ capacity1 };

    std::size_t const capacity2{ 3 };
    tt::ringbuf<int> buf2{ capacity2 };

    swap(buf1, buf2);
    REQUIRE_EQ(capacity1, buf2.capacity());
    REQUIRE_EQ(capacity2, buf1.capacity());
}

TEST_CASE("ringbuf::emplace_back/pop_front with overwrite")
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

TEST_CASE("lock_free_ringbuf empty/move ctor")
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

TEST_CASE("lock_free_ringbuf::push_back/pop_front")
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
TEST_CASE("lock_free_ringbuf::emplace_back/pop_front")
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

TEST_CASE("lock_free_ringbuf::emplace_back/pop_front several times")
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

TEST_CASE("lock_free_ringbuf::emplace_back/pop_front with overwrite")
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
void
produce(tt::lock_free_ringbuf<int>& buf, std::atomic<int>& tasks_count)
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
void
consume(tt::lock_free_ringbuf<T>& buf, std::atomic<int>& tasks_count,
        std::atomic<std::size_t>& consumed_count)
{
    for (;;)
    {
        if (tasks_count.load() < 0 && buf.empty()) break;
        auto const res = buf.pop_front();
        if (res) consumed_count.fetch_add(1);
    }
};

TEST_CASE("lock_free_ringbuf produce/consume")
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
