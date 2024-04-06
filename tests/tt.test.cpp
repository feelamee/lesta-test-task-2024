#include <tt/tt.hpp>

#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <utility>

struct fail_info
{
    std::source_location source_info;
    std::string          msg;
};

template <>
struct std::formatter<std::source_location>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(std::source_location const& obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(),
                              "{}:{}:{} in '{}'",
                              obj.file_name(),
                              obj.line(),
                              obj.column(),
                              obj.function_name());
    }
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
        if ((!expr))                                                                               \
            return fail_info{ std::source_location::current(), "'" #expr "' is... lie" };          \
    }

namespace tests
{

std::optional<fail_info>
iseven()
{
    REQUIRE(true == tt::iseven(0));
    REQUIRE(true == tt::iseven(2));
    REQUIRE(true == tt::iseven(-2));

    REQUIRE(false == tt::iseven(1));
    REQUIRE(false == tt::iseven(-1));
    REQUIRE(false == tt::iseven(3));

    return std::nullopt;
}

} // namespace tests

template <std::invocable Fn>
int
run(Fn&& fn)
{
    if (auto mbinfo = std::invoke(std::forward<Fn>(fn)); mbinfo)
    {
        std::clog << std::format("[TEST FAILED] {}\n", mbinfo.value());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int
main()
{
    return run(&tests::iseven);
}
