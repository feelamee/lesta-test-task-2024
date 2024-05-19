#pragma once

#include <tt/detail.hpp>

#include <algorithm>
#include <compare>
#include <iterator>
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

    using pointer = allocator_traits::pointer;
    using const_pointer = allocator_traits::const_pointer;
    using reference = value_type&;
    using const_reference = value_type const&;

    using difference_type = allocator_traits::difference_type;
    using size_type = allocator_traits::size_type;

private:
    void
    init_with_capacity_and_allocator(size_type sz, allocator_type const& alloc = allocator_type())
    {
        m_allocator = alloc;

        m_buf = allocator_traits::allocate(get_allocator(), sz);
        m_end = m_buf + sz;

        m_last = m_buf;
        m_first = m_buf;

        m_size = 0;
    }

public:
    ringbuf(size_type sz, allocator_type const& alloc = allocator_type())
        : m_allocator{ alloc }
    {
        init_with_capacity_and_allocator(sz, alloc);
    }

    ringbuf(this_type const& other)
    {
        init_with_capacity_and_allocator(other.capacity(), other.get_allocator());
        assign(std::ranges::ref_view(other));
    }
    ringbuf(this_type&& other) noexcept
    {
        init_with_capacity_and_allocator(other.capacity(), other.get_allocator());
        assign(std::ranges::owning_view(std::forward<this_type>(other)));
    }

    this_type&
    operator=(this_type other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    ~ringbuf()
    {
        clear();
        allocator_traits::deallocate(m_allocator, m_buf, capacity());
    }

    allocator_type
    get_allocator() const noexcept
    {
        return m_allocator;
    }

    allocator_type&
    get_allocator() noexcept
    {
        return m_allocator;
    }

    size_type
    size() const noexcept
    {
        return m_size;
    }

    size_type
    capacity() const noexcept
    {
        return m_end - m_buf;
    }

    size_type
    max_size() const noexcept
    {
        return allocator_traits::max_size(m_allocator);
    }

    bool
    empty() const noexcept
    {
        return 0 == size();
    }

    bool
    full() const noexcept
    {
        return capacity() == size();
    }

    size_type
    reserve() const noexcept
    {
        return capacity() - size();
    }

    void
    assign(std::ranges::input_range auto&& other)
    {
        clear();
        append_range(std::forward<decltype(other)>(other));
    }

    friend void
    swap(this_type& lhs, this_type& rhs) noexcept(noexcept(swap(lhs.m_allocator, rhs.m_allocator)))
    {
        using std::swap;

        swap(lhs.m_buf, rhs.m_buf);
        swap(lhs.m_end, rhs.m_end);
        swap(lhs.m_allocator, rhs.m_allocator);

        swap(lhs.m_last, rhs.m_last);
        swap(lhs.m_first, rhs.m_first);
        swap(lhs.m_size, rhs.m_size);
    }

    void
    push_back(param_value_type value)
    {
        emplace_back(value);
    }

    void
    emplace_back(auto&&... args)
        requires std::is_constructible_v<value_type, decltype(args)...>
    {
        if (full())
        {
            if (empty()) return;

            (*m_last) = value_type{ std::forward<decltype(args)>(args)... };
            increment(m_last);
            m_first = m_last;
        } else
        {
            allocator_traits::construct(get_allocator(), m_last,
                                        std::forward<decltype(args)>(args)...);
            increment(m_last);
            ++m_size;
        }
    }

    // TODO: specialize for `std::ranges::sized_range` using `drop(size(view) -
    // capacity())`
    template <std::ranges::input_range R>
    void
    append_range(std::ranges::ref_view<R> view)
    {
        std::ranges::copy(view, std::back_inserter(*this));
    }

    template <std::ranges::input_range R>
    void
    append_range(std::ranges::owning_view<R> view)
    {
        std::ranges::move(view, std::back_inserter(*this));
    }

    std::optional<value_type>
    pop_back()
    {
        if (empty()) return std::nullopt;

        value_type ret{ *(--end()) };
        decrement(m_last);
        allocator_traits::destroy(get_allocator(), m_last);
        --m_size;
        return ret;
    }

    std::optional<value_type>
    pop_front()
    {
        if (empty()) return std::nullopt;

        value_type ret{ *begin() };
        allocator_traits::destroy(get_allocator(), m_first);
        increment(m_first);
        --m_size;
        return ret;
    }

    void
    clear() noexcept
    {
        using std::ranges::destroy_n;

        std::destroy(begin(), end());
        m_first = m_buf;
        m_last = m_buf;
        m_size = 0;
    }

    template <bool IsConst>
    struct iterator;

    iterator<false>
    begin() noexcept
    {
        return { this, empty() ? nullptr : m_first };
    }

    iterator<true>
    begin() const noexcept
    {
        return { this, empty() ? nullptr : m_first };
    }

    iterator<false>
    end() noexcept
    {
        return { this, nullptr };
    }

    iterator<true>
    end() const noexcept
    {
        return { this, nullptr };
    }

    iterator<true>
    cbegin() const noexcept
    {
        return begin();
    }

    iterator<true>
    cend() const noexcept
    {
        return end();
    }

    friend bool
    operator==(this_type const& lhs, this_type const& rhs)
    {
        if (lhs.size() != rhs.size()) return false;

        return std::ranges::equal(lhs, rhs);
    }

private:
    void
    increment(pointer& p) const
    {
        if (++p == m_end) p = m_buf;
    }

    void
    decrement(pointer& p) const
    {
        if (p == m_buf) p = m_end;
        --p;
    }

    template <typename Pointer>
    Pointer
    add(Pointer p, difference_type n) const
    {
        return p + (n < (m_end - p) ? n : n - capacity());
    }

    template <class Pointer>
    Pointer
    sub(Pointer p, difference_type n) const
    {
        return p - (n > (p - m_buf) ? n - capacity() : n);
    }

    pointer m_buf{ nullptr };
    pointer m_end{ nullptr };
    allocator_type m_allocator;

    pointer m_last{ nullptr };
    pointer m_first{ nullptr };
    size_type m_size{ 0 };
};

template <std::semiregular T, typename Alloc>
template <bool IsConst>
struct ringbuf<T, Alloc>::iterator
{
private:
    using container_type = ringbuf<T, Alloc>;

public:
    using this_type = iterator;
    using const_self = iterator<true>;
    using value_type = container_type::value_type;
    using difference_type = container_type::difference_type;
    using reference =
        std::conditional_t<IsConst, container_type::const_reference, container_type::reference>;
    using pointer =
        std::conditional_t<IsConst, container_type::const_pointer, container_type::pointer>;

    pointer m_ptr{ nullptr };
    const container_type* m_buf{ nullptr };

    iterator() = default;
    iterator(container_type const* const p_buf, value_type* p_ptr)
        : m_ptr{ p_ptr }
        , m_buf{ p_buf }
    {
    }

    iterator(this_type const&) = default;
    iterator(this_type&&) = default;
    iterator& operator=(this_type const&) = default;
    iterator& operator=(this_type&&) = default;

    friend this_type&
    operator+=(this_type& it, difference_type n)
    {
        if (n > 0)
        {
            it.m_ptr = it.m_buf->add(it.m_ptr, n);
            if (it.m_ptr == it.m_buf->m_last) it.m_ptr = nullptr;
        } else if (n < 0)
        {
            it -= -n;
        }
        return it;
    }

    friend iterator
    operator+(this_type it, difference_type n)
    {
        return it += n;
    }

    friend iterator
    operator+(difference_type n, this_type it)
    {
        return it + n;
    }

    this_type&
    operator-=(difference_type n)
    {
        if (n > 0)
        {
            m_ptr = m_buf->sub(nullptr == m_ptr ? m_buf->m_last : m_ptr, n);
        } else if (n < 0)
        {
            *this += -n;
        }
        return *this;
    }

    friend iterator
    operator-(this_type it, difference_type n)
    {
        return it -= n;
    }

    friend difference_type
    operator-(this_type lhs, this_type rhs)
    {
        return linearize_pointer(lhs) - linearize_pointer(rhs);
    }

    reference
    operator[](size_type n) const noexcept
    {
        return *((*this) + n);
    }

    friend std::strong_ordering // weak or strong?
    operator<=>(this_type lhs, this_type rhs)
    {
        return linearize_pointer(lhs) <=> linearize_pointer(rhs);
    }

    this_type&
    operator--()
    {
        return (*this) -= 1;
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
        return *m_ptr;
    }

    reference
    operator->()
    {
        return (operator*());
    }

    this_type&
    operator++()
    {
        return (*this) += 1;
    }

    this_type
    operator++(int)
    {
        this_type ret{ *this };
        ++(*this);
        return ret;
    }

    friend bool operator==(this_type, this_type) = default;

    friend bool
    operator!=(this_type l, this_type r)
    {
        return not(l == r);
    }

private:
    template <bool pIsConst>
    static iterator<pIsConst>::pointer
    linearize_pointer(iterator<pIsConst> const it)
    {
        return it.m_ptr == 0 ? it.m_buf->m_buf + it.m_buf->size()
                             : (it.m_ptr < it.m_buf->m_first
                                    ? it.m_ptr + (it.m_buf->m_end - it.m_buf->m_first)
                                    : it.m_buf->m_buf + (it.m_ptr - it.m_buf->m_first));
    }
};

static_assert(std::random_access_iterator<ringbuf<int>::iterator<false>>);
static_assert(std::random_access_iterator<ringbuf<int>::iterator<true>>);
static_assert(std::sentinel_for<ringbuf<int>::iterator<false>, ringbuf<int>::iterator<false>>);
static_assert(std::sentinel_for<ringbuf<int>::iterator<true>, ringbuf<int>::iterator<true>>);
static_assert(std::ranges::range<tt::ringbuf<int>&>);

} // namespace tt
