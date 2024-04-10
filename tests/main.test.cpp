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

#define REQUIRE(expr)                                                                              \
    {                                                                                              \
        if (!(expr))                                                                               \
            return fail_info{ std::source_location::current(), "'" #expr "' is... lie" };          \
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
    REQUIRE(true == tt::iseven(0));

    REQUIRE(true == tt::iseven(2));
    REQUIRE(true == tt::iseven(-2));

    REQUIRE(false == tt::iseven(1));
    REQUIRE(false == tt::iseven(-1));
    REQUIRE(false == tt::iseven(3));

    return std::nullopt;
},

+[]() -> std::optional<fail_info>
{
    {
        tt::ringbuf<int> buf(0);
        REQUIRE(buf.empty());
        REQUIRE(0 == buf.size());
        REQUIRE(buf.full());
    }
    {
        using value_type = int;
        using allocator_type = std::allocator<value_type>;
        using allocator_traits = std::allocator_traits<allocator_type>;

        {
            allocator_type alloc;
            tt::ringbuf<value_type, allocator_type> buf(0, alloc);
            REQUIRE(buf.max_size() == allocator_traits::max_size(alloc));
        }

        {
            struct custom_allocator : public std::allocator<value_type>
            {
                int id;
            };
            custom_allocator alloc1{ .id = 42 };
            custom_allocator alloc2{ .id = 69 };
            tt::ringbuf<value_type, custom_allocator> buf(42, alloc1);
            REQUIRE(alloc1.id == buf.get_allocator().id);
            REQUIRE(alloc2.id != buf.get_allocator().id);
        }
    }
    {
        tt::ringbuf<int> buf(1);
        // REQUIRE(buf.begin() == buf.end());
    }
    {
        // tt::ringbuf<int> buf1(1);
        {
            // tt::ringbuf<int> buf2 = buf1;
            // buf2.emplace(5);
            // REQUIRE(buf1 != buf2);
        }
        {
            // auto buf2 = buf1;
            // buf2.emplace(5);
            // auto buf3 = buf2;
            // buf2.emplace(5);
            // REQUIRE(buf3 == buf2);
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
