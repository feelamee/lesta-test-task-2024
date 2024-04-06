#include <tt/tt.hpp> // tt::iseven

#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS, std::size_t
#include <format>      // std::format
#include <functional>  // std::invoke
#include <iostream>    // std::log
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward

struct fail_info
{
    std::size_t line{ 0 };
    std::string_view filename;
    std::string_view function;
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
        return std::format_to(ctx.out(),
                              "{}:{} in {}\nLastly he said: '{}'",
                              obj.filename,
                              obj.line,
                              obj.function,
                              obj.msg);
    }
};

#define fail(message)                                                          \
    std::make_optional(fail_info{ .line = __LINE__,                            \
                                  .filename = __FILE__,                        \
                                  .function = __func__,                        \
                                  .msg = (message) })

#define REQUIRE(expr)                                                          \
    {                                                                          \
        if ((!expr))                                                           \
            return fail("'" #expr "' is... lie");                              \
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
