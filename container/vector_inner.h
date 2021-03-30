#ifndef _CONTAINER_VECTOR_INNER_H_
#define _CONTAINER_VECTOR_INNER_H_

#include <memory>

namespace ak_vector_inner {

/// See bits/stl_deque.h's _Deque_base for an explanation.
template <typename _Tp, typename _Alloc>
struct _Vector_base {
  using _Tp_alloc_type = typename __gnu_cxx::__alloc_traits<_Alloc>::template rebind<_Tp>::other;
  using pointer = typename __gnu_cxx::__alloc_traits<_Tp_alloc_type>::pointer;

  struct _Vector_impl_data {
    pointer _M_start;
    pointer _M_finish;
    pointer _M_end_of_storage;

    _Vector_impl_data() noexcept : _M_start(), _M_finish(), _M_end_of_storage() {}

    _Vector_impl_data(_Vector_impl_data &&__x) noexcept : _M_start(__x._M_start), _M_finish(__x._M_finish), _M_end_of_storage(__x._M_end_of_storage) {
      __x._M_start = __x._M_finish = __x._M_end_of_storage = pointer();
    }

    void _M_copy_data(_Vector_impl_data const &__x) noexcept {
      _M_start = __x._M_start;
      _M_finish = __x._M_finish;
      _M_end_of_storage = __x._M_end_of_storage;
    }

    void _M_swap_data(_Vector_impl_data &__x) noexcept {
      // Do not use std::swap(_M_start, __x._M_start), etc as it loses
      // information used by TBAA.
      _Vector_impl_data __tmp;
      __tmp._M_copy_data(*this);
      _M_copy_data(__x);
      __x._M_copy_data(__tmp);
    }
  };

  struct _Vector_impl : public _Tp_alloc_type, public _Vector_impl_data {
    _Vector_impl() noexcept(std::is_nothrow_default_constructible<_Tp_alloc_type>::value)

        : _Tp_alloc_type() {}

    _Vector_impl(_Tp_alloc_type const &__a) noexcept : _Tp_alloc_type(__a) {}

    // Not defaulted, to enforce noexcept(true) even when
    // !is_nothrow_move_constructible<_Tp_alloc_type>.
    _Vector_impl(_Vector_impl &&__x) noexcept : _Tp_alloc_type(std::move(__x)), _Vector_impl_data(std::move(__x)) {}

    _Vector_impl(_Tp_alloc_type &&__a) noexcept : _Tp_alloc_type(std::move(__a)) {}

    _Vector_impl(_Tp_alloc_type &&__a, _Vector_impl &&__rv) noexcept : _Tp_alloc_type(std::move(__a)), _Vector_impl_data(std::move(__rv)) {}

    // NOTE: AddressSanitizer (aka ASan) IS NOT AVAILABLE IN THIS IMPLEMENTATION
#define _GLIBCXX_ASAN_ANNOTATE_REINIT
#define _GLIBCXX_ASAN_ANNOTATE_GROW(n)
#define _GLIBCXX_ASAN_ANNOTATE_GREW(n)
#define _GLIBCXX_ASAN_ANNOTATE_SHRINK(n)
#define _GLIBCXX_ASAN_ANNOTATE_BEFORE_DEALLOC
  };

 public:
  using allocator_type = _Alloc;

  _Tp_alloc_type &_M_get_Tp_allocator() noexcept { return this->_M_impl; }

  const _Tp_alloc_type &_M_get_Tp_allocator() const noexcept { return this->_M_impl; }

  allocator_type get_allocator() const noexcept { return allocator_type(_M_get_Tp_allocator()); }

  _Vector_base() = default;

  _Vector_base(const allocator_type &__a) noexcept : _M_impl(__a) {}

  // Kept for ABI compatibility.
  _Vector_base(size_t __n) : _M_impl() { _M_create_storage(__n); }

  _Vector_base(size_t __n, const allocator_type &__a) : _M_impl(__a) { _M_create_storage(__n); }

  _Vector_base(_Vector_base &&) = default;

  // Kept for ABI compatibility.
  _Vector_base(_Tp_alloc_type &&__a) noexcept : _M_impl(std::move(__a)) {}

  _Vector_base(_Vector_base &&__x, const allocator_type &__a) : _M_impl(__a) {
    if (__x.get_allocator() == __a)
      this->_M_impl._M_swap_data(__x._M_impl);
    else {
      size_t __n = __x._M_impl._M_finish - __x._M_impl._M_start;
      _M_create_storage(__n);
    }
  }

  _Vector_base(const allocator_type &__a, _Vector_base &&__x) : _M_impl(_Tp_alloc_type(__a), std::move(__x._M_impl)) {}

  ~_Vector_base() noexcept { _M_deallocate(_M_impl._M_start, _M_impl._M_end_of_storage - _M_impl._M_start); }

 public:
  _Vector_impl _M_impl;

  pointer _M_allocate(size_t __n) {
    typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Tr;
    return __n != 0 ? _Tr::allocate(_M_impl, __n) : pointer();
  }
  void _M_deallocate(pointer __p, size_t __n) {
    typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Tr;
    if (__p) _Tr::deallocate(_M_impl, __p, __n);
  }

 protected:
  void _M_create_storage(size_t __n) {
    this->_M_impl._M_start = this->_M_allocate(__n);
    this->_M_impl._M_finish = this->_M_impl._M_start;
    this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __n;
  }
};

}  // namespace ak_vector_inner

#endif
