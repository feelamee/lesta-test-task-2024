#include <tt/tt.hpp>

#include <cassert>
#include <cstdlib>

int
main()
{
    assert(false == tt::iseven(5));
    assert(true == tt::iseven(0));
    assert(false == tt::iseven(1));
    return EXIT_SUCCESS;
}
