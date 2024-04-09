#pragma once

#include <memory>
#include <tt/detail.hpp>

#include <algorithm>
#include <ranges>
#include <utility>

namespace tt
{

template <std::semiregular T, typename Alloc = std::allocator<T>>
class ringbuf
{
public:
    using this_type = ringbuf<T, Alloc>;
    using allocator_type = Alloc;

    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = allocator_traits::value_type;
    // using param_value_type = detail::param<value_type>;
    // using rvalue_type      = value_type&&;

    using pointer = allocator_traits::pointer;
    // using const_pointer   = allocator_traits::const_pointer;
    // using reference = allocator_traits::reference;
    // using const_reference = allocator_traits::const_reference;

    using difference_type = allocator_traits::difference_type;
    using size_type = allocator_traits::size_type;

    ringbuf(size_type sz, allocator_type const& alloc = allocator_type())
        : bufsize{ sz }, allocator{ alloc }
    {
        buf = allocator_traits::allocate(allocator, sz);
        write = buf;
        read = buf;
    }

    ringbuf(this_type const&) { detail::unimplemented(); }
    ringbuf(this_type&&) noexcept { detail::unimplemented(); }

    this_type&
    operator=(this_type other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    ~ringbuf()
    {
        clear();
        allocator_traits::deallocate(allocator, buf, bufsize);
    }

    allocator_type
    get_allocator() const noexcept
    {
        return allocator;
    }

    allocator_type&
    get_allocator() noexcept
    {
        return allocator;
    }

    size_type
    size() const noexcept
    {
        difference_type const diff = write - read;
        return diff >= 0 ? diff : bufsize + diff;
    }

    size_type
    max_size() const noexcept
    {
        return allocator_traits::max_size(allocator);
    }

    bool
    empty() const noexcept
    {
        return write == read;
    }

    bool
    full() const noexcept
    {
        difference_type const diff = write - read;
        return std::cmp_equal(diff, (diff >= 0 ? bufsize : -1));
    }
    size_type
    reserve() const noexcept
    {
        return bufsize - size();
    }

    template <std::ranges::input_range R>
    void
    assign(std::ranges::ref_view<R>)
    {
        detail::unimplemented();
    }

    template <std::ranges::input_range R>
    void
    assign(std::ranges::owning_view<R>)
    {
        detail::unimplemented();
    }

    friend void
    swap(this_type& lhs, this_type& rhs) noexcept(noexcept(swap(lhs.allocator, rhs.allocator)))
    {
        using std::swap;

        swap(lhs.buf, rhs.buf);
        swap(lhs.bufsize, rhs.bufsize);
        swap(lhs.allocator, rhs.allocator);

        swap(lhs.write, rhs.write);
        swap(lhs.read, rhs.read);
    }

    template <std::ranges::input_range R>
    void
    push(std::ranges::ref_view<R>)
    {
        detail::unimplemented();
    }

    template <std::ranges::input_range R>
    void
    push(std::ranges::owning_view<R>)
    {
        detail::unimplemented();
    }

    std::ranges::owning_view<std::span<value_type>>
    pop(size_type)
    {
        detail::unimplemented();
    }

    void
    clear() noexcept
    {
        using std::ranges::destroy_n;

        difference_type const diff = write - read;
        if (diff > 0)
        {
            destroy_n(read, diff);
        }
        else if (diff < 0)
        {
            destroy_n(buf, write - buf);
            destroy_n(read, bufsize - (read - buf));
        }
    }

    friend bool
    operator==(this_type const&, this_type const&)
    {
        detail::unimplemented();
    }

private:
    pointer buf{ nullptr };
    size_type bufsize{ 0 };
    allocator_type allocator;

    pointer write{ nullptr };
    pointer read{ nullptr };
};

} // namespace tt
