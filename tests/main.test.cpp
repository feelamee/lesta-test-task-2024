#include <tt/iseven.hpp>
#include <tt/lock_free_ringbuf.hpp>
#include <tt/ringbuf.hpp>

#include <array>
#include <format>
#include <functional>
#include <optional>

struct fail_info
{
    std::source_location source_info;
    std::string msg;
};

template <>
struct std::formatter<fail_info>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(fail_info const& obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}\nLastly he said: '{}'", obj.source_info, obj.msg);
    }
};

template <typename T>
struct std::formatter<std::optional<T>>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(std::optional<T> const& obj, std::format_context& ctx) const
    {
        if (obj)
        {
            return std::format_to(ctx.out(), "?{}", *obj);
        } else
        {
            return std::format_to(ctx.out(), "?{}", "none");
        }
    }
};

template <std::semiregular T, typename Alloc>
struct std::formatter<tt::ringbuf<T, Alloc>>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(tt::ringbuf<T, Alloc> const& obj, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "[ ");
        for (auto const& el : obj)
        {
            std::format_to(ctx.out(), "{} ", el);
        }
        return std::format_to(ctx.out(), "]");
    }
};

template <template <bool> typename Iter, bool IsConst>
struct std::formatter<Iter<IsConst>>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(Iter<IsConst> const obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{{ .m_buf = {}, .m_ptr = {} }}", (void*)obj.m_buf,
                              (void*)obj.m_ptr);
    }
};

#define REQUIRE(expr)                                                                              \
    {                                                                                              \
        auto const expr_result = (expr);                                                           \
        if (not expr_result)                                                                       \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("{} is... lie", expr) };                                 \
    }

#define REQUIRE_EQ(expr1, expr2)                                                                   \
    {                                                                                              \
        auto const expr1_result = (expr1);                                                         \
        auto const expr2_result = (expr2);                                                         \
        if (not(expr1_result == expr2_result))                                                     \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("{} == {} is... lie", expr1, expr2) };                   \
    }

#define REQUIRE_NEQ(expr1, expr2)                                                                  \
    {                                                                                              \
        auto const expr1_result = (expr1);                                                         \
        auto const expr2_result = (expr2);                                                         \
        if (not(expr1_result != expr2_result))                                                     \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("{} != {} is... lie", expr1, expr2) };                   \
    }

constexpr auto run = []<std::invocable Fn>(Fn&& fn) -> int
{
    if (auto mbinfo = std::invoke(std::forward<Fn>(fn)); mbinfo)
    {
        std::clog << std::format("[TEST FAILED] {}\n", mbinfo.value());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
};

using test_return_type = std::optional<fail_info>;
// here I missing pretty name of functions in log when test failed,
// but also I miss any chance to forget call test function from main()
// althrough I'm still have filename:line:column
constexpr std::array tests = {

    // iseven
    +[]() -> test_return_type
    {
        REQUIRE_EQ(true, tt::iseven(0));

        REQUIRE_EQ(true, tt::iseven(2));
        REQUIRE_EQ(true, tt::iseven(-2));

        REQUIRE_EQ(false, tt::iseven(1));
        REQUIRE_EQ(false, tt::iseven(-1));
        REQUIRE_EQ(false, tt::iseven(3));

        return std::nullopt;
    },

    // tt::ringbuf
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 0 };
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());
        REQUIRE(buf.full());
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf1{ 0 };
        tt::ringbuf<int> buf2{ 1 };
        REQUIRE_EQ(buf1, buf2);
        return std::nullopt;
    },
    +[]() -> test_return_type
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
            REQUIRE_NEQ(alloc2.id, buf.get_allocator().id);
        }
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 1 };
        REQUIRE_EQ(buf.begin(), buf.end());

        int const val{ 42 };
        buf.emplace_back(val);
        REQUIRE_EQ(buf.size(), 1);
        REQUIRE_EQ(buf.begin() + 1, buf.end());
        REQUIRE_EQ(*buf.begin(), val);
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf1{ 1 };
        REQUIRE(buf1.empty());
        buf1.emplace_back(5);
        REQUIRE(buf1.full());
        buf1.clear();
        REQUIRE(buf1.empty());
        {
            tt::ringbuf<int> buf2{ buf1 };
            REQUIRE_EQ(buf2.capacity(), 1)

            buf2.emplace_back(5);
            REQUIRE_NEQ(buf1, buf2);
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
            REQUIRE_NEQ(buf2, buf3);
            buf3 = buf2;
            REQUIRE_EQ(buf2, buf3);
        }
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 2 };
        buf.emplace_back(42);
        auto const v{ buf.pop_back() };
        REQUIRE(v.has_value())
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 2 };
        auto const v{ buf.pop_back() };
        REQUIRE(not v.has_value())
        REQUIRE_EQ(std::optional<int>(), v);
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 2 };
        buf.emplace_back(42);
        auto const v{ buf.pop_front() };
        REQUIRE(v.has_value())
        REQUIRE_EQ(42, *v);
        REQUIRE(buf.empty());
        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        tt::ringbuf<int> buf{ 2 };
        auto const v{ buf.pop_back() };
        REQUIRE(not v.has_value())
        REQUIRE_EQ(std::optional<int>(), v);
        return std::nullopt;
    },
    +[]() -> std::optional<fail_info>
    {
        std::size_t const capacity1{ 5 };
        tt::ringbuf<int> buf1{ capacity1 };

        using std::views::all;
        buf1.append_range(std::array{ 1, 2, 3, 4, 5, 6 } | all);
        std::array expected{ 2, 3, 4, 5, 6 };
        auto i{ buf1.begin() };
        auto j{ expected.begin() };
        for (; i != buf1.end() && j != expected.end(); ++i, ++j) REQUIRE_EQ(*i, *j);

        return std::nullopt;
    },
    +[]() -> test_return_type
    {
        std::size_t const capacity1{ 5 };
        tt::ringbuf<int> buf1{ capacity1 };

        std::size_t const capacity2{ 3 };
        tt::ringbuf<int> buf2{ capacity2 };

        swap(buf1, buf2);
        REQUIRE_EQ(capacity1, buf2.capacity());
        REQUIRE_EQ(capacity2, buf1.capacity());
        return std::nullopt;
    },

    // tt::ringbuf_v2
    +[]() -> test_return_type
    {
        std::size_t capacity{ 2 };
        REQUIRE(tt::detail::is_power_of_2(capacity));

        tt::lock_free_ringbuf<int> buf{ capacity };
        REQUIRE_EQ(capacity, buf.capacity())
        REQUIRE(buf.empty());
        REQUIRE(!buf.full());

        tt::lock_free_ringbuf<int> buf2{ std::move(buf) };
        REQUIRE_EQ(capacity, buf2.capacity())
        REQUIRE(buf2.empty());
        REQUIRE(!buf2.full());

        return std::nullopt;
    },
    +[]() -> test_return_type
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

        return std::nullopt;
    }

};

int
main()
{
    return std::ranges::any_of(tests, run);
}
