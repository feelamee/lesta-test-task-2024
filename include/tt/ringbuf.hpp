#pragma once

#include <algorithm>
#include <tt/detail.hpp>

#include <concepts>
#include <optional>
#include <ranges>

namespace tt
{

/*
 */

template <std::semiregular T, typename Alloc = std::allocator<T>>
class ringbuf
{
public:
    using this_type      = ringbuf<T, Alloc>;
    using allocator_type = Alloc;

    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type       = allocator_traits::value_type;
    // using param_value_type = detail::param<value_type>;
    // using rvalue_type      = value_type&&;

    using pointer = allocator_traits::pointer;
    // using const_pointer   = allocator_traits::const_pointer;
    // using reference = allocator_traits::reference;
    // using const_reference = allocator_traits::const_reference;

    using difference_type = allocator_traits::difference_type;
    using size_type       = allocator_traits::size_type;

    ringbuf(size_type sz, allocator_type const& alloc = allocator_type())
        : bufsize{ sz }, allocator{ alloc }
    {
        buf   = allocator_traits::allocate(allocator, sz);
        write = buf;
        read  = buf;
    }

    ringbuf(this_type const&);
    ringbuf(this_type&&) noexcept;

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
        difference_type diff = write - read;
        return diff > 0 ? diff : bufsize - diff;
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
        difference_type diff = write - read;
        return diff == (diff < 0 ? 1 : bufsize);
    }
    size_type
    reserve() const noexcept
    {
        return bufsize - size();
    }

    template <std::ranges::input_range R>
    void assign(std::ranges::ref_view<R>);

    template <std::ranges::input_range R>
    void assign(std::ranges::owning_view<R>);

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
    void push(std::ranges::ref_view<R>);

    template <std::ranges::input_range R>
    void push(std::ranges::owning_view<R>);

    std::ranges::owning_view<std::span<value_type>> pop(size_type);

    void
    clear() noexcept
    {
        difference_type diff = write - read;
        if (diff > 0)
        {
            destroy(read, diff);
        }
        else if (diff < 0)
        {
            destroy(buf, write - buf);
            destroy(read, bufsize - (read - buf));
        }
    }

private:
    void
    destroy(pointer const p, size_type const n) noexcept
    {
        std::for_each(p, p + n, [this](pointer ptr) { allocator_traits::destroy(allocator, ptr); });
    }

    pointer        buf{ nullptr };
    size_type      bufsize{ 0 };
    allocator_type allocator;

    pointer write{ nullptr };
    pointer read{ nullptr };
};

} // namespace tt
