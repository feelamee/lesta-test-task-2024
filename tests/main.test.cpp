#include <cmath>
#include <tt/iseven.hpp>
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
        return std::format_to(
            ctx.out(), "{{ .m_buf = {}, .m_ptr = {} }}", (void*)obj.m_buf, (void*)obj.m_ptr);
    }
};

#define REQUIRE(expr)                                                                              \
    {                                                                                              \
        auto const expr_result = (expr);                                                           \
        if (not expr_result)                                                                       \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("'{}' is... lie", expr) };                               \
    }

#define REQUIRE_EQ(expr1, expr2)                                                                   \
    {                                                                                              \
        auto const expr1_result = (expr1);                                                         \
        auto const expr2_result = (expr2);                                                         \
        if (not(expr1_result == expr2_result))                                                     \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("'{} == {}' is... lie", expr1, expr2) };                 \
    }

#define REQUIRE_NEQ(expr1, expr2)                                                                  \
    {                                                                                              \
        auto const expr1_result = (expr1);                                                         \
        auto const expr2_result = (expr2);                                                         \
        if (not(expr1_result != expr2_result))                                                     \
            return fail_info{ std::source_location::current(),                                     \
                              std::format("'{} != {}' is... lie", expr1, expr2) };                 \
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

// here I missing pretty name of functions in log when test failed,
// but also I miss any chance to forget call test function from main()
constexpr std::array tests = {

    // clang-format off
+[]() -> std::optional<fail_info>
{
    REQUIRE_EQ(true, tt::iseven(0));

    REQUIRE_EQ(true, tt::iseven(2));
    REQUIRE_EQ(true, tt::iseven(-2));

    REQUIRE_EQ(false, tt::iseven(1));
    REQUIRE_EQ(false, tt::iseven(-1));
    REQUIRE_EQ(false, tt::iseven(3));

    return std::nullopt;
},

+[]() -> std::optional<fail_info>
{
    {
        tt::ringbuf<int> buf(0);
        REQUIRE(buf.empty());
        REQUIRE_EQ(0, buf.size());
        REQUIRE(buf.full());
    }
    {
        tt::ringbuf<int> buf1(0);
        tt::ringbuf<int> buf2(1);
        REQUIRE_EQ(buf1, buf2);
    }
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
    }
    {
        tt::ringbuf<int> buf(1);
        REQUIRE_EQ(buf.begin(), buf.end());
        REQUIRE_EQ(buf.size(), 0);

        buf.emplace_back(42);
        REQUIRE_EQ(buf.size(), 1);
        REQUIRE_NEQ(buf.begin(), buf.end());
        REQUIRE_EQ(buf.begin() + 1, buf.end());
    }
    {
        tt::ringbuf<int> buf1(1);
        buf1.emplace_back(5);
        REQUIRE_EQ(buf1.size(), 1);
        buf1.clear();
        REQUIRE_EQ(buf1.size(), 0);
        {
            tt::ringbuf<int> buf2 = buf1;
            REQUIRE(buf2.empty())
            REQUIRE_EQ(buf2.capacity(), 1)
            REQUIRE(not buf2.full())

            buf2.emplace_back(5);
            REQUIRE(buf2.full())
            REQUIRE(not buf2.empty())
            REQUIRE_EQ(buf2.size(), 1);
            REQUIRE_NEQ(buf1, buf2);
        }
        {
            auto buf2 = buf1;
            buf2.emplace_back(5);
            auto buf3 = buf2;
            buf2.emplace_back(5);
            REQUIRE_EQ(buf3, buf2);
        }

    }
    return std::nullopt;
}
    // clang-format on
};

int
main()
{
    return std::ranges::any_of(tests, run);
}
