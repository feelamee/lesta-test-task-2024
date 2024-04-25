#pragma once

#include <tt/detail.hpp>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <ranges>

namespace tt
{

template <std::semiregular T, typename Alloc = std::allocator<T>>
class lock_free_ringbuf
{
public:
    using this_type = lock_free_ringbuf<T, Alloc>;

    using allocator_type = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = allocator_traits::value_type;
    using param_value_type = detail::param<value_type>;
    using pointer = allocator_traits::pointer;
    using size_type = allocator_traits::size_type;

private:
    struct value_type_impl
    {
        value_type value;
        std::size_t counter{ 0 };
    };
    using allocator_type_impl = allocator_traits::template rebind_alloc<value_type_impl>;
    using allocator_traits_impl = std::allocator_traits<allocator_type_impl>;
    using pointer_impl = allocator_traits_impl::pointer;

private:
private:
    void
    init_with_capacity(size_type sz)
    {
        m_buf_begin = allocator_traits_impl::allocate(get_allocator_impl(), sz);
        m_buf_end = m_buf_begin + sz;

        m_last.store(0, std::memory_order::seq_cst);
        m_first.store(0, std::memory_order::seq_cst);
    }

public:
    ///! @pre `sz` is power of 2
    lock_free_ringbuf(size_type sz, allocator_type const& alloc = allocator_type())
        : m_allocator(alloc)
    {
        assert(detail::is_power_of_2(sz));
        init_with_capacity(sz);
    }

    lock_free_ringbuf(this_type&& other) noexcept
        : m_allocator(other.get_allocator())
    {
        init_with_capacity(other.capacity());
        assign(std::forward<this_type>(other));
    }

    this_type&
    operator=(this_type other) noexcept
    {
        assign(std::move(other));
    }

    ~lock_free_ringbuf()
    {
        clear();
        allocator_traits_impl::deallocate(get_allocator_impl(), m_buf_begin, capacity());
    }

    void
    assign(this_type&& other)
    {
        clear();
        while (!other.empty()) push_back(*other.pop_front());
    }

    void
    clear() noexcept
    {
        while (size())
        {
            (void)pop_front();
        }
        m_first.store(0, std::memory_order::seq_cst);
        m_last.store(0, std::memory_order::seq_cst);
    }

    allocator_type
    get_allocator() const noexcept
    {
        return allocator_type(get_allocator_impl());
    }

    size_type
    size() const noexcept
    {
        std::int64_t const diff{
            static_cast<std::int64_t>(m_last.load(std::memory_order::seq_cst)) -
            static_cast<std::int64_t>(m_first.load(std::memory_order::seq_cst))
        };
        return diff >= 0 ? diff : capacity() - diff;
    }

    size_type
    capacity() const noexcept
    {
        return m_buf_end - m_buf_begin;
    }

    size_type
    max_size() const noexcept
    {
        return allocator_traits_impl::max_size(get_allocator_impl());
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
    push_back(param_value_type v)
    {
        emplace_back(value_type{ v });
    }

    void
    emplace_back(auto&&... args)
        requires std::is_constructible_v<value_type, decltype(args)...>
    {
        detail::unimplemented();
    }

    std::optional<value_type>
    pop_front()
    {
        detail::unimplemented();
    }

    // TODO: specialize for `std:ranges::sized_range` using `drop(size(view) -
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

private:
    void
    increment(std::atomic<pointer>&) const
    {
        detail::unimplemented();
    }

    allocator_type_impl&
    get_allocator_impl()
    {
        return m_allocator;
    }

    allocator_type_impl
    get_allocator_impl() const
    {
        return m_allocator;
    }

    pointer_impl m_buf_begin{ nullptr };
    pointer_impl m_buf_end{ nullptr };

    std::atomic<size_type> m_last{ 0 };
    std::atomic<size_type> m_first{ 0 };

    // TODO: use inheritance to optimize size of ringbuf,
    //       if allocator_type is stateless
    allocator_type_impl m_allocator;

    static_assert(decltype(m_last)::is_always_lock_free);
    static_assert(decltype(m_first)::is_always_lock_free);
};

} // namespace tt
