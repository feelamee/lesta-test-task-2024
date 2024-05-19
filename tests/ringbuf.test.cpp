#include <tt/ringbuf.hpp>

#include "doctest.h"

#include <ranges>

TEST_SUITE("ringbuf")
{
    TEST_CASE("empty/size/full")
    {
        tt::ringbuf<int> buf{ 0 };
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());
        REQUIRE(buf.full());
    }

    TEST_CASE("operator==")
    {
        tt::ringbuf<int> buf1{ 0 };
        tt::ringbuf<int> buf2{ 1 };
        REQUIRE_EQ(buf1, buf2);
    }

    TEST_CASE("max_size")
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
            custom_allocator alloc1;
            alloc1.id = 42; // missing initializer on gcc when using designated initializer
            custom_allocator alloc2;
            alloc1.id = 69; // missing initializer on gcc when using designated initializer
            tt::ringbuf<value_type, custom_allocator> buf(42, alloc1);
            REQUIRE_EQ(alloc1.id, buf.get_allocator().id);
            REQUIRE_NE(alloc2.id, buf.get_allocator().id);
        }
    }

    TEST_CASE("emplace_back")
    {
        tt::ringbuf<int> buf{ 1 };
        REQUIRE_EQ(buf.begin(), buf.end());

        int const val{ 42 };
        buf.emplace_back(val);
        REQUIRE_EQ(buf.size(), 1);
        REQUIRE_EQ(buf.begin() + 1, buf.end());
        REQUIRE_EQ(*buf.begin(), val);
    }

    TEST_CASE("emplace_back with overwrite")
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

    TEST_CASE("emplace_back/pop_back")
    {
        tt::ringbuf<int> buf{ 2 };
        buf.emplace_back(42);
        auto const v{ buf.pop_back() };
        REQUIRE(v.has_value());
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
    }

    TEST_CASE("pop_back on empty")
    {
        tt::ringbuf<int> buf{ 2 };
        auto const v{ buf.pop_back() };
        REQUIRE(!v.has_value());
        REQUIRE_EQ(std::optional<int>(), v);
    }

    TEST_CASE("emplace_back/pop_front")
    {
        tt::ringbuf<int> buf{ 2 };
        buf.emplace_back(42);
        auto const v{ buf.pop_front() };
        REQUIRE(v.has_value());
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
    }

    TEST_CASE("append_range")
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

    TEST_CASE("swap")
    {
        std::size_t const capacity1{ 5 };
        tt::ringbuf<int> buf1{ capacity1 };

        std::size_t const capacity2{ 3 };
        tt::ringbuf<int> buf2{ capacity2 };

        swap(buf1, buf2);
        REQUIRE_EQ(capacity1, buf2.capacity());
        REQUIRE_EQ(capacity2, buf1.capacity());
    }

    TEST_CASE("emplace_back/pop_front with overwrite")
    {
        std::size_t const capacity1{ 1 };
        tt::ringbuf<int> buf{ capacity1 };
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
}
