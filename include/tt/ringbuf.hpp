#pragma once

#include <iterator>
#include <tt/detail.hpp>

#include <algorithm>
#include <memory>
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
    using param_value_type = detail::param<value_type>;
    using rvalue_type = value_type&&;

    using pointer = value_type*;
    using const_pointer = value_type const*;
    using reference = value_type&;
    using const_reference = value_type const&;

    using difference_type = allocator_traits::difference_type;
    using size_type = allocator_traits::size_type;

    ringbuf(size_type sz, allocator_type const& alloc = allocator_type())
        : bufsize{ sz }, allocator{ alloc }
    {
        buf = allocator_traits::allocate(allocator, sz);
        towrite = buf;
        toread = buf;
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
        difference_type const diff = towrite - toread;
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
        return towrite == toread;
    }

    bool
    full() const noexcept
    {
        difference_type const diff = towrite - toread;
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

        swap(lhs.towrite, rhs.towrite);
        swap(lhs.toread, rhs.toread);
    }

    void
    emplace(const_reference v)
    {
        emplace(value_type{ v });
    }

    void
    emplace(auto&&... v)
        requires std::is_constructible_v<value_type, decltype(v)...>
    {
        if (towrite == toread)
        {
            (*toread) = value_type{ std::forward<decltype(v)>(v)... };
        }
        else
        {
            allocator_traits::construct(allocator, towrite, std::forward<decltype(v)>(v)...);
        }
        towrite = next(towrite);
        // detail::unimplemented();
    }

    void
    emplace(std::ranges::input_range auto const& rng)
    {
        std::ranges::for_each(rng, [](auto const&) { detail::unimplemented(); });
    }

    void
    emplace(std::ranges::input_range auto&& rng)
    {
        std::ranges::for_each(rng, [](auto&&) { detail::unimplemented(); });
    }

    value_type
    pop(size_type)
    {
        detail::unimplemented();
    }

    void
    clear() noexcept
    {
        using std::ranges::destroy_n;

        difference_type const diff = towrite - toread;
        if (diff > 0)
        {
            destroy_n(toread, diff);
        }
        else if (diff < 0)
        {
            destroy_n(buf, towrite - buf);
            destroy_n(toread, bufsize - (toread - buf));
        }
    }

    struct iterator;

    iterator
    begin() const noexcept
    {
        detail::unimplemented();
    }

    iterator
    end() const noexcept
    {
        detail::unimplemented();
    }

    friend bool
    operator==(this_type const& lhs, this_type const& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        detail::unimplemented();
    }

private:
    // too bad...
    constexpr pointer
    next(pointer p) const
    {
        if (nullptr == p)
            return buf;

        ++p;
        if (p > buf + bufsize)
            p -= bufsize;

        return p;
    }

    pointer buf{ nullptr };
    size_type bufsize{ 0 };
    allocator_type allocator;

    pointer towrite{ nullptr };
    pointer toread{ nullptr };
};

template <std::semiregular T, typename Alloc>
struct ringbuf<T, Alloc>::iterator
{
    using this_type = iterator;
    using container_type = ringbuf<T, Alloc>;

    using value_type = container_type::value_type;
    using difference_type = container_type::difference_type;
    using reference = container_type::reference;
    using pointer = container_type::pointer;
    // using size_type = allocator_traits::size_type;

    reference
    operator*() const
    {
        detail::unimplemented();
    }

    pointer
    operator->()
    {
        detail::unimplemented();
    }

    this_type&
    operator++()
    {
        detail::unimplemented();
    }

    this_type
    operator++(int)
    {
        detail::unimplemented();
    }

    this_type&
    operator--()
    {
        detail::unimplemented();
    }

    this_type
    operator--(int)
    {
        detail::unimplemented();
    }

    friend difference_type
    operator-(this_type, this_type)
    {
        detail::unimplemented();
    };

    friend iterator
    operator+(this_type, difference_type)
    {
        detail::unimplemented();
    };

    friend iterator
    operator+(difference_type, this_type)
    {
        detail::unimplemented();
    };

    friend iterator
    operator-(this_type, difference_type)
    {
        detail::unimplemented();
    };

    this_type&
    operator+=(difference_type)
    {
        detail::unimplemented();
    };

    this_type&
    operator-=(difference_type)
    {
        detail::unimplemented();
    };

    friend bool
    operator==(this_type, this_type)
    {
        detail::unimplemented();
    };

    friend bool
    operator!=(this_type, this_type)
    {
        detail::unimplemented();
    };

    reference
    operator[](size_type) const noexcept
    {
        detail::unimplemented();
    };

    friend constexpr auto operator<=>(this_type, this_type) = default;
};
static_assert(std::random_access_iterator<ringbuf<int>::iterator>);

} // namespace tt
