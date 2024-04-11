#pragma once

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

    ringbuf(this_type const& other) { assign(other); }
    ringbuf(this_type&& other) noexcept { assign(std::forward<this_type>(other)); }

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
    assign(std::ranges::ref_view<R> other)
    {
        clear();
        for (auto const& i : other)
            emplace(i);
    }

    template <std::ranges::input_range R>
    void
    assign(std::ranges::owning_view<R> other)
    {
        clear();
        for (auto&& i : other)
            emplace(std::forward<decltype(i)>(i));
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
    emplace(auto&&... args)
        requires std::is_constructible_v<value_type, decltype(args)...>
    {
        detail::unimplemented();
        if (towrite == toread)
        {
            (*toread) = value_type{ std::forward<decltype(args)>(args)... };
        }
        else
        {
            allocator_traits::construct(allocator, towrite, std::forward<decltype(args)>(args)...);
        }
        towrite = next(towrite);
    }

    template <std::ranges::input_range R>
    void
    emplace(std::ranges::ref_view<R> view)
    {
        std::ranges::for_each(view, [this](auto const& i) { emplace(i); });
    }

    template <std::ranges::input_range R>
    void
    emplace(std::ranges::owning_view<R> view)
    {
        std::ranges::for_each(view, [this](auto&& i) { emplace(std::forward<decltype(i)>(i)); });
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
        toread = nullptr;
        towrite = nullptr;
    }

    template <bool IsConst>
    struct iterator;

    iterator<false>
    begin() const noexcept
    {
        return iterator{ read };
    }

    iterator<false>
    end() const noexcept
    {
        return iterator{ write };
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
template <bool IsConst>
struct ringbuf<T, Alloc>::iterator
{
private:
    using container_type = ringbuf<T, Alloc>;

public:
    using this_type = iterator;
    using value_type = container_type::value_type;
    using difference_type = container_type::difference_type;
    using reference = detail::const_if<IsConst, container_type::reference>;
    using pointer = container_type::pointer;

    iterator() = default;
    iterator(pointer p_p) : p{ p_p } {}

    this_type&
    operator+=(difference_type n)
    {
        p += n;
        return *this;
    };

    friend iterator
    operator+(this_type it, difference_type n)
    {
        auto temp = it;
        return it += n;
    };

    friend iterator
    operator+(difference_type n, this_type it)
    {
        return it + n;
    };

    this_type&
    operator-=(difference_type n)
    {
        return (*this) += -n;
    };

    friend iterator
    operator-(this_type it, difference_type n)
    {
        return it + (-n);
    };

    friend difference_type
    operator-(this_type l, this_type r)
    {
        assert(l == r + (l - r));
        return l - r;
    };

    reference
    operator[](size_type i) const noexcept
    {
        return *((*this) + i);
    };

    friend constexpr auto operator<=>(this_type, this_type) = default;

    this_type&
    operator--()
    {
        --p;
        return *this;
    }

    this_type
    operator--(int)
    {
        this_type ret{ *this };
        --(*this);
        return ret;
    }

    reference
    operator*() const
    {
        return *p;
    }

    reference
    operator->()
    {
        return *p;
    }

    this_type&
    operator++()
    {

        return ++p;
    }

    this_type
    operator++(int)
    {
        this_type ret{ p };
        ++(*this);
        return ret;
    }

    friend bool
    operator==(this_type l, this_type r)
    {
        return l.p == r.p;
    };

    friend bool
    operator!=(this_type l, this_type r)
    {
        return not(l == r);
    };

private:
    pointer p{ nullptr };
};
static_assert(std::random_access_iterator<ringbuf<int>::iterator<false>>);
static_assert(std::random_access_iterator<ringbuf<int>::iterator<true>>);
static_assert(std::sentinel_for<ringbuf<int>::iterator<false>, ringbuf<int>::iterator<false>>);
static_assert(std::sentinel_for<ringbuf<int>::iterator<true>, ringbuf<int>::iterator<true>>);

} // namespace tt
