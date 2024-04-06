#pragma once

#include <tt/detail.hpp>

#include <concepts>
#include <iterator>
#include <memory>

namespace tt
{

template <std::semiregular T, typename Alloc = std::allocator<T>>
class ringbuf
{
public:
    using this_type      = ringbuf<T, Alloc>;
    using allocator_type = Alloc;

    using value_type       = std::allocator_traits<allocator_type>::value_type;
    using param_value_type = detail::param_t<value_type> const;
    using rvalue_type      = value_type&&;

    using pointer         = std::allocator_traits<allocator_type>::pointer;
    using const_pointer   = std::allocator_traits<allocator_type>::const_pointer;
    using reference       = std::allocator_traits<allocator_type>::reference;
    using const_reference = std::allocator_traits<allocator_type>::const_reference;

    using difference_type = std::allocator_traits<allocator_type>::difference_type;
    using size_type       = std::allocator_traits<allocator_type>::size_type;
    using capacity_type   = size_type;

    using iterator               = std::iterator_traits<this_type>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_iterator         = std::iterator_traits<this_type>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    explicit ringbuf(allocator_type const& = allocator_type()) noexcept;
    explicit ringbuf(capacity_type, allocator_type const& = allocator_type());
    ringbuf(size_type, param_value_type, allocator_type const& = allocator_type());
    ringbuf(capacity_type, size_type, param_value_type, allocator_type const& = allocator_type());
    ringbuf(this_type const&);
    ringbuf(this_type&&) noexcept;

    template <std::input_iterator It>
    ringbuf(It, It, allocator_type const& = allocator_type());
    template <std::input_iterator It>
    ringbuf(capacity_type, It, It, allocator_type const& = allocator_type());

    this_type& operator=(const this_type&);
    this_type& operator=(this_type&&) noexcept;
    ~ringbuf();

    allocator_type  get_allocator() const noexcept;
    allocator_type& get_allocator() noexcept;

    iterator         begin() noexcept;
    reverse_iterator rbegin() noexcept;
    iterator         end() noexcept;
    reverse_iterator rend() noexcept;

    const_iterator         begin() const noexcept;
    const_iterator         end() const noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;

    reference       operator[](size_type);
    const_reference operator[](size_type) const;

    reference       at(size_type);
    const_reference at(size_type) const;

    reference front();
    reference back();

    const_reference front() const;
    const_reference back() const;

    size_type     size() const noexcept;
    size_type     max_size() const noexcept;
    bool          empty() const noexcept;
    bool          full() const noexcept;
    size_type     reserve() const noexcept;
    capacity_type capacity() const noexcept;
    void          set_capacity(capacity_type);
    void          resize(size_type, param_value_type = value_type());
    void          assign(size_type, param_value_type);
    template <std::input_iterator It>
    void assign(It, It);
    void swap(this_type&) noexcept;

    void     push_back(param_value_type);
    void     push_back(rvalue_type);
    void     push_back();
    void     push_front(param_value_type);
    void     push_front(rvalue_type);
    void     push_front();
    void     pop_back();
    void     pop_front();
    iterator insert(iterator, param_value_type);
    iterator insert(iterator, rvalue_type);
    iterator insert(iterator);
    void     insert(iterator, size_type, param_value_type);
    template <std::input_iterator It>
    void     insert(iterator, It, It);
    iterator erase(iterator);
    iterator erase(iterator, iterator);
    void     clear() noexcept;

private:
};

} // namespace tt
