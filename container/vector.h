#ifndef _CONTAINER_VECTOR_TEST_H_
#define _CONTAINER_VECTOR_TEST_H_

/**
 * Container requirements:
 * https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer
 * https://gcc.gnu.org/onlinedocs/gcc-10.2.0/libstdc++/api/tables.html
 */

#include "vector_inner.h"

namespace ak_container {

template <typename _Tp, typename _Alloc>
using std_vector_base = typename ak_vector_inner::template _Vector_base<_Tp, _Alloc>;
///////////////////////////////////////////////////////////////////

/**
 *  @brief A standard container which offers fixed time access to
 *  individual elements in any order.
 *
 *  @ingroup sequences
 *
 *  @tparam _Tp  Type of element.
 *  @tparam _Alloc  Allocator type, defaults to allocator<_Tp>.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and a
 *  <a href="tables.html#67">sequence</a>, including the
 *  <a href="tables.html#68">optional sequence requirements</a> with the
 *  %exception of @c push_front and @c pop_front.
 *
 *  In some terminology a %vector can be described as a dynamic
 *  C-style array, it offers fast and efficient access to individual
 *  elements in any order and saves the user from worrying about
 *  memory and size allocation.  Subscripting ( @c [] ) access is
 *  also provided as with C-style arrays.
 */
template <typename _Tp, typename _Alloc = ::std::allocator<_Tp>>
class vector : protected ak_vector_inner::_Vector_base<_Tp, _Alloc> {
  static_assert(std::is_same<typename std::remove_cv<_Tp>::type, _Tp>::value, "std::vector must have a non-const, non-volatile value_type");

  static_assert(std::is_same<typename _Alloc::value_type, _Tp>::value, "std::vector must have the same value_type as its allocator");

  using _Base = typename ak_vector_inner::template _Vector_base<_Tp, _Alloc>;
  using _Tp_alloc_type = typename _Base::_Tp_alloc_type;
  using _Alloc_traits = __gnu_cxx::__alloc_traits<_Tp_alloc_type>;

 public:
  using value_type = _Tp;
  using pointer = typename _Base::pointer;
  using const_pointer = typename _Alloc_traits::const_pointer;
  using reference = typename _Alloc_traits::reference;
  using const_reference = typename _Alloc_traits::const_reference;
  using iterator = __gnu_cxx::__normal_iterator<pointer, vector>;
  using const_iterator = __gnu_cxx::__normal_iterator<const_pointer, vector>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using allocator_type = _Alloc;

 private:
  static constexpr bool _S_nothrow_relocate(std::true_type) {
    return noexcept(std::__relocate_a(std::declval<pointer>(), std::declval<pointer>(), std::declval<pointer>(), std::declval<_Tp_alloc_type &>()));
  }

  static constexpr bool _S_nothrow_relocate(std::false_type) { return false; }

  static constexpr bool _S_use_relocate() {
    // Instantiating std::__relocate_a might cause an error outside the
    // immediate context (in __relocate_object_a's noexcept-specifier),
    // so only do it if we know the type can be move-inserted into *this.
    return _S_nothrow_relocate(std::__is_move_insertable<_Tp_alloc_type>{});
  }

  static pointer _S_do_relocate(pointer __first, pointer __last, pointer __result, _Tp_alloc_type &__alloc, std::true_type) noexcept {
    return std::__relocate_a(__first, __last, __result, __alloc);
  }

  static pointer _S_do_relocate(pointer, pointer, pointer __result, _Tp_alloc_type &, std::false_type) noexcept { return __result; }

  static pointer _S_relocate(pointer __first, pointer __last, pointer __result, _Tp_alloc_type &__alloc) noexcept {
    using __do_it = std::__bool_constant<_S_use_relocate()>;
    return _S_do_relocate(__first, __last, __result, __alloc, __do_it{});
  }

 protected:
  using _Base::_M_allocate;
  using _Base::_M_deallocate;
  using _Base::_M_get_Tp_allocator;
  using _Base::_M_impl;

 public:
  // [23.2.4.1] construct/copy/destroy
  // (assign() and get_allocator() are also listed in this section)

  /**
   *  @brief  Creates a %vector with no elements.
   */
  vector() = default;

  /**
   *  @brief  Creates a %vector with no elements.
   *  @param  __a  An allocator object.
   */
  explicit vector(const allocator_type &__a) noexcept : _Base(__a) {}

  /**
   *  @brief  Creates a %vector with default constructed elements.
   *  @param  __n  The number of elements to initially create.
   *  @param  __a  An allocator.
   *
   *  This constructor fills the %vector with @a __n default
   *  constructed elements.
   */
  explicit vector(size_type __n, const allocator_type &__a = allocator_type()) : _Base(_S_check_init_len(__n, __a), __a) { _M_default_initialize(__n); }

  /**
   *  @brief  Creates a %vector with copies of an exemplar element.
   *  @param  __n  The number of elements to initially create.
   *  @param  __value  An element to copy.
   *  @param  __a  An allocator.
   *
   *  This constructor fills the %vector with @a __n copies of @a __value.
   */
  vector(size_type __n, const value_type &__value, const allocator_type &__a = allocator_type()) : _Base(_S_check_init_len(__n, __a), __a) {
    _M_fill_initialize(__n, __value);
  }

  /**
   *  @brief  %Vector copy constructor.
   *  @param  __x  A %vector of identical element and allocator types.
   *
   *  All the elements of @a __x are copied, but any unused capacity in
   *  @a __x  will not be copied
   *  (i.e. capacity() == size() in the new %vector).
   *
   *  The newly-created %vector uses a copy of the allocator object used
   *  by @a __x (unless the allocator traits dictate a different object).
   */
  vector(const vector &__x) : _Base(__x.size(), _Alloc_traits::_S_select_on_copy(__x._M_get_Tp_allocator())) {
    this->_M_impl._M_finish = std::__uninitialized_copy_a(__x.begin(), __x.end(), this->_M_impl._M_start, _M_get_Tp_allocator());
  }

  /**
   *  @brief  %Vector move constructor.
   *
   *  The newly-created %vector contains the exact contents of the
   *  moved instance.
   *  The contents of the moved instance are a valid, but unspecified
   *  %vector.
   */
  vector(vector &&) noexcept = default;

  /// Copy constructor with alternative allocator
  vector(const vector &__x, const allocator_type &__a) : _Base(__x.size(), __a) {
    this->_M_impl._M_finish = std::__uninitialized_copy_a(__x.begin(), __x.end(), this->_M_impl._M_start, _M_get_Tp_allocator());
  }

 private:
  vector(vector &&__rv, const allocator_type &__m, std::true_type) noexcept : _Base(__m, std::move(__rv)) {}

  vector(vector &&__rv, const allocator_type &__m, std::false_type) : _Base(__m) {
    if (__rv.get_allocator() == __m)
      this->_M_impl._M_swap_data(__rv._M_impl);
    else if (!__rv.empty()) {
      this->_M_create_storage(__rv.size());
      this->_M_impl._M_finish = std::__uninitialized_move_a(__rv.begin(), __rv.end(), this->_M_impl._M_start, _M_get_Tp_allocator());
      __rv.clear();
    }
  }

 public:
  /// Move constructor with alternative allocator
  vector(vector &&__rv, const allocator_type &__m) noexcept(noexcept(vector(std::declval<vector &&>(), std::declval<const allocator_type &>(),
                                                                            std::declval<typename _Alloc_traits::is_always_equal>())))
      : vector(std::move(__rv), __m, typename _Alloc_traits::is_always_equal{}) {}

  /**
   *  @brief  Builds a %vector from an initializer list.
   *  @param  __l  An initializer_list.
   *  @param  __a  An allocator.
   *
   *  Create a %vector consisting of copies of the elements in the
   *  initializer_list @a __l.
   *
   *  This will call the element type's copy constructor N times
   *  (where N is @a __l.size()) and do no memory reallocation.
   */
  vector(std::initializer_list<value_type> __l, const allocator_type &__a = allocator_type()) : _Base(__a) {
    _M_range_initialize(__l.begin(), __l.end(), std::random_access_iterator_tag());
  }

  /**
   *  @brief  Builds a %vector from a range.
   *  @param  __first  An input iterator.
   *  @param  __last  An input iterator.
   *  @param  __a  An allocator.
   *
   *  Create a %vector consisting of copies of the elements from
   *  [first,last).
   *
   *  If the iterators are forward, bidirectional, or
   *  random-access, then this will call the elements' copy
   *  constructor N times (where N is distance(first,last)) and do
   *  no memory reallocation.  But if only input iterators are
   *  used, then this will do at most 2N calls to the copy
   *  constructor, and logN memory reallocations.
   */
  template <typename _InputIterator, typename = std::_RequireInputIter<_InputIterator>>
  vector(_InputIterator __first, _InputIterator __last, const allocator_type &__a = allocator_type()) : _Base(__a) {
    _M_range_initialize(__first, __last, std::__iterator_category(__first));
  }

  /**
   *  The dtor only erases the elements, and note that if the
   *  elements themselves are pointers, the pointed-to memory is
   *  not touched in any way.  Managing the pointer is the user's
   *  responsibility.
   */
  ~vector() noexcept { std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator()); }

  /**
   *  @brief  %Vector assignment operator.
   *  @param  __x  A %vector of identical element and allocator types.
   *
   *  All the elements of @a __x are copied, but any unused capacity in
   *  @a __x will not be copied.
   *
   *  Whether the allocator is copied depends on the allocator traits.
   */
  vector &operator=(const vector &__x);

  /**
   *  @brief  %Vector move assignment operator.
   *  @param  __x  A %vector of identical element and allocator types.
   *
   *  The contents of @a __x are moved into this %vector (without copying,
   *  if the allocators permit it).
   *  Afterwards @a __x is a valid, but unspecified %vector.
   *
   *  Whether the allocator is moved depends on the allocator traits.
   */
  vector &operator=(vector &&__x) noexcept(_Alloc_traits::_S_nothrow_move()) {
    constexpr bool __move_storage = _Alloc_traits::_S_propagate_on_move_assign() || _Alloc_traits::_S_always_equal();
    _M_move_assign(std::move(__x), std::__bool_constant<__move_storage>());
    return *this;
  }

  /**
   *  @brief  %Vector list assignment operator.
   *  @param  __l  An initializer_list.
   *
   *  This function fills a %vector with copies of the elements in the
   *  initializer list @a __l.
   *
   *  Note that the assignment completely changes the %vector and
   *  that the resulting %vector's size is the same as the number
   *  of elements assigned.
   */
  vector &operator=(std::initializer_list<value_type> __l) {
    this->_M_assign_aux(__l.begin(), __l.end(), std::random_access_iterator_tag());
    return *this;
  }

  /**
   *  @brief  Assigns a given value to a %vector.
   *  @param  __n  Number of elements to be assigned.
   *  @param  __val  Value to be assigned.
   *
   *  This function fills a %vector with @a __n copies of the given
   *  value.  Note that the assignment completely changes the
   *  %vector and that the resulting %vector's size is the same as
   *  the number of elements assigned.
   */
  void assign(size_type __n, const value_type &__val) { _M_fill_assign(__n, __val); }

  /**
   *  @brief  Assigns a range to a %vector.
   *  @param  __first  An input iterator.
   *  @param  __last   An input iterator.
   *
   *  This function fills a %vector with copies of the elements in the
   *  range [__first,__last).
   *
   *  Note that the assignment completely changes the %vector and
   *  that the resulting %vector's size is the same as the number
   *  of elements assigned.
   */
  template <typename _InputIterator, typename = std::_RequireInputIter<_InputIterator>>
  void assign(_InputIterator __first, _InputIterator __last) {
    _M_assign_dispatch(__first, __last, std::__false_type());
  }

  /**
   *  @brief  Assigns an initializer list to a %vector.
   *  @param  __l  An initializer_list.
   *
   *  This function fills a %vector with copies of the elements in the
   *  initializer list @a __l.
   *
   *  Note that the assignment completely changes the %vector and
   *  that the resulting %vector's size is the same as the number
   *  of elements assigned.
   */
  void assign(std::initializer_list<value_type> __l) { this->_M_assign_aux(__l.begin(), __l.end(), std::random_access_iterator_tag()); }

  /// Get a copy of the memory allocation object.
  using _Base::get_allocator;

  // iterators
  /**
   *  Returns a read/write iterator that points to the first
   *  element in the %vector.  Iteration is done in ordinary
   *  element order.
   */
  iterator begin() noexcept { return iterator(this->_M_impl._M_start); }

  /**
   *  Returns a read-only (constant) iterator that points to the
   *  first element in the %vector.  Iteration is done in ordinary
   *  element order.
   */
  const_iterator begin() const noexcept { return const_iterator(this->_M_impl._M_start); }

  /**
   *  Returns a read/write iterator that points one past the last
   *  element in the %vector.  Iteration is done in ordinary
   *  element order.
   */
  iterator end() noexcept { return iterator(this->_M_impl._M_finish); }

  /**
   *  Returns a read-only (constant) iterator that points one past
   *  the last element in the %vector.  Iteration is done in
   *  ordinary element order.
   */
  const_iterator end() const noexcept { return const_iterator(this->_M_impl._M_finish); }

  /**
   *  Returns a read/write reverse iterator that points to the
   *  last element in the %vector.  Iteration is done in reverse
   *  element order.
   */
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  /**
   *  Returns a read-only (constant) reverse iterator that points
   *  to the last element in the %vector.  Iteration is done in
   *  reverse element order.
   */
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

  /**
   *  Returns a read/write reverse iterator that points to one
   *  before the first element in the %vector.  Iteration is done
   *  in reverse element order.
   */
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  /**
   *  Returns a read-only (constant) reverse iterator that points
   *  to one before the first element in the %vector.  Iteration
   *  is done in reverse element order.
   */
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  /**
   *  Returns a read-only (constant) iterator that points to the
   *  first element in the %vector.  Iteration is done in ordinary
   *  element order.
   */
  const_iterator cbegin() const noexcept { return const_iterator(this->_M_impl._M_start); }

  /**
   *  Returns a read-only (constant) iterator that points one past
   *  the last element in the %vector.  Iteration is done in
   *  ordinary element order.
   */
  const_iterator cend() const noexcept { return const_iterator(this->_M_impl._M_finish); }

  /**
   *  Returns a read-only (constant) reverse iterator that points
   *  to the last element in the %vector.  Iteration is done in
   *  reverse element order.
   */
  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

  /**
   *  Returns a read-only (constant) reverse iterator that points
   *  to one before the first element in the %vector.  Iteration
   *  is done in reverse element order.
   */
  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

  // [23.2.4.2] capacity
  /**  Returns the number of elements in the %vector.  */
  size_type size() const noexcept { return size_type(this->_M_impl._M_finish - this->_M_impl._M_start); }

  /**  Returns the size() of the largest possible %vector.  */
  size_type max_size() const noexcept { return _S_max_size(_M_get_Tp_allocator()); }

  /**
   *  @brief  Resizes the %vector to the specified number of elements.
   *  @param  __new_size  Number of elements the %vector should contain.
   *
   *  This function will %resize the %vector to the specified
   *  number of elements.  If the number is smaller than the
   *  %vector's current size the %vector is truncated, otherwise
   *  default constructed elements are appended.
   */
  void resize(size_type __new_size) {
    if (__new_size > size())
      _M_default_append(__new_size - size());
    else if (__new_size < size())
      _M_erase_at_end(this->_M_impl._M_start + __new_size);
  }

  /**
   *  @brief  Resizes the %vector to the specified number of elements.
   *  @param  __new_size  Number of elements the %vector should contain.
   *  @param  __x  Data with which new elements should be populated.
   *
   *  This function will %resize the %vector to the specified
   *  number of elements.  If the number is smaller than the
   *  %vector's current size the %vector is truncated, otherwise
   *  the %vector is extended and new elements are populated with
   *  given data.
   */
  void resize(size_type __new_size, const value_type &__x) {
    if (__new_size > size())
      _M_fill_insert(end(), __new_size - size(), __x);
    else if (__new_size < size())
      _M_erase_at_end(this->_M_impl._M_start + __new_size);
  }

  /**  A non-binding request to reduce capacity() to size().  */
  void shrink_to_fit() { _M_shrink_to_fit(); }

  /**
   *  Returns the total number of elements that the %vector can
   *  hold before needing to allocate more memory.
   */
  size_type capacity() const noexcept { return size_type(this->_M_impl._M_end_of_storage - this->_M_impl._M_start); }

  /**
   *  Returns true if the %vector is empty.  (Thus begin() would
   *  equal end().)
   */
  [[__nodiscard__]] bool empty() const noexcept { return begin() == end(); }

  /**
   *  @brief  Attempt to preallocate enough memory for specified number of
   *          elements.
   *  @param  __n  Number of elements required.
   *  @throw  std::length_error  If @a n exceeds @c max_size().
   *
   *  This function attempts to reserve enough memory for the
   *  %vector to hold the specified number of elements.  If the
   *  number requested is more than max_size(), length_error is
   *  thrown.
   *
   *  The advantage of this function is that if optimal code is a
   *  necessity and the user can determine the number of elements
   *  that will be required, the user can reserve the memory in
   *  %advance, and thus prevent a possible reallocation of memory
   *  and copying of %vector data.
   */
  void reserve(size_type __n);

  // element access
  /**
   *  @brief  Subscript access to the data contained in the %vector.
   *  @param __n The index of the element for which data should be
   *  accessed.
   *  @return  Read/write reference to data.
   *
   *  This operator allows for easy, array-style, data access.
   *  Note that data access with this operator is unchecked and
   *  out_of_range lookups are not defined. (For checked lookups
   *  see at().)
   */
  reference operator[](size_type __n) noexcept { return *(this->_M_impl._M_start + __n); }

  /**
   *  @brief  Subscript access to the data contained in the %vector.
   *  @param __n The index of the element for which data should be
   *  accessed.
   *  @return  Read-only (constant) reference to data.
   *
   *  This operator allows for easy, array-style, data access.
   *  Note that data access with this operator is unchecked and
   *  out_of_range lookups are not defined. (For checked lookups
   *  see at().)
   */
  const_reference operator[](size_type __n) const noexcept { return *(this->_M_impl._M_start + __n); }

 protected:
  /// Safety check used only from at().
  void _M_range_check(size_type __n) const {
    if (__n >= this->size())
      __throw_out_of_range_fmt(("vector::_M_range_check: __n "
                                "(which is %zu) >= this->size() "
                                "(which is %zu)"),
                               __n, this->size());
  }

 public:
  /**
   *  @brief  Provides access to the data contained in the %vector.
   *  @param __n The index of the element for which data should be
   *  accessed.
   *  @return  Read/write reference to data.
   *  @throw  std::out_of_range  If @a __n is an invalid index.
   *
   *  This function provides for safer data access.  The parameter
   *  is first checked that it is in the range of the vector.  The
   *  function throws out_of_range if the check fails.
   */
  reference at(size_type __n) {
    _M_range_check(__n);
    return (*this)[__n];
  }

  /**
   *  @brief  Provides access to the data contained in the %vector.
   *  @param __n The index of the element for which data should be
   *  accessed.
   *  @return  Read-only (constant) reference to data.
   *  @throw  std::out_of_range  If @a __n is an invalid index.
   *
   *  This function provides for safer data access.  The parameter
   *  is first checked that it is in the range of the vector.  The
   *  function throws out_of_range if the check fails.
   */
  const_reference at(size_type __n) const {
    _M_range_check(__n);
    return (*this)[__n];
  }

  /**
   *  Returns a read/write reference to the data at the first
   *  element of the %vector.
   */
  reference front() noexcept { return *begin(); }

  /**
   *  Returns a read-only (constant) reference to the data at the first
   *  element of the %vector.
   */
  const_reference front() const noexcept { return *begin(); }

  /**
   *  Returns a read/write reference to the data at the last
   *  element of the %vector.
   */
  reference back() noexcept { return *(end() - 1); }

  /**
   *  Returns a read-only (constant) reference to the data at the
   *  last element of the %vector.
   */
  const_reference back() const noexcept { return *(end() - 1); }

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // DR 464. Suggestion for new member functions in standard containers.
  // data access
  /**
   *   Returns a pointer such that [data(), data() + size()) is a valid
   *   range.  For a non-empty %vector, data() == &front().
   */
  _Tp *data() noexcept { return _M_data_ptr(this->_M_impl._M_start); }

  const _Tp *data() const noexcept { return _M_data_ptr(this->_M_impl._M_start); }

  // [23.2.4.3] modifiers
  /**
   *  @brief  Add data to the end of the %vector.
   *  @param  __x  Data to be added.
   *
   *  This is a typical stack operation.  The function creates an
   *  element at the end of the %vector and assigns the given data
   *  to it.  Due to the nature of a %vector this operation can be
   *  done in constant time if the %vector has preallocated space
   *  available.
   */
  void push_back(const value_type &__x) {
    if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage) {
      _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, __x);
      ++this->_M_impl._M_finish;
    } else if (grow(1))
      push_back(__x);
    else
      _M_realloc_insert(end(), __x);
  }
  void push_back(value_type &&__x) { emplace_back(std::move(__x)); }
  template <typename... _Args>
  reference emplace_back(_Args &&...__args);

  /**
   *  @brief  Removes last element.
   *
   *  This is a typical stack operation. It shrinks the %vector by one.
   *
   *  Note that no data is returned, and if the last element's
   *  data is needed, it should be retrieved before pop_back() is
   *  called.
   */
  void pop_back() noexcept {
    --this->_M_impl._M_finish;
    _Alloc_traits::destroy(this->_M_impl, this->_M_impl._M_finish);
  }

  /**
   *  @brief  Inserts an object in %vector before specified iterator.
   *  @param  __position  A const_iterator into the %vector.
   *  @param  __args  Arguments.
   *  @return  An iterator that points to the inserted data.
   *
   *  This function will insert an object of type T constructed
   *  with T(std::forward<Args>(args)...) before the specified location.
   *  Note that this kind of operation could be expensive for a %vector
   *  and if it is frequently used the user should consider using
   *  std::list.
   */
  template <typename... _Args>
  iterator emplace(const_iterator __position, _Args &&...__args) {
    return _M_emplace_aux(__position, std::forward<_Args>(__args)...);
  }

  /**
   *  @brief  Inserts given value into %vector before specified iterator.
   *  @param  __position  A const_iterator into the %vector.
   *  @param  __x  Data to be inserted.
   *  @return  An iterator that points to the inserted data.
   *
   *  This function will insert a copy of the given value before
   *  the specified location.  Note that this kind of operation
   *  could be expensive for a %vector and if it is frequently
   *  used the user should consider using std::list.
   */
  iterator insert(const_iterator __position, const value_type &__x);

  /**
   *  @brief  Inserts given rvalue into %vector before specified iterator.
   *  @param  __position  A const_iterator into the %vector.
   *  @param  __x  Data to be inserted.
   *  @return  An iterator that points to the inserted data.
   *
   *  This function will insert a copy of the given rvalue before
   *  the specified location.  Note that this kind of operation
   *  could be expensive for a %vector and if it is frequently
   *  used the user should consider using std::list.
   */
  iterator insert(const_iterator __position, value_type &&__x) { return _M_insert_rval(__position, std::move(__x)); }

  /**
   *  @brief  Inserts an initializer_list into the %vector.
   *  @param  __position  An iterator into the %vector.
   *  @param  __l  An initializer_list.
   *
   *  This function will insert copies of the data in the
   *  initializer_list @a l into the %vector before the location
   *  specified by @a position.
   *
   *  Note that this kind of operation could be expensive for a
   *  %vector and if it is frequently used the user should
   *  consider using std::list.
   */
  iterator insert(const_iterator __position, std::initializer_list<value_type> __l) {
    auto __offset = __position - cbegin();
    _M_range_insert(begin() + __offset, __l.begin(), __l.end(), std::random_access_iterator_tag());
    return begin() + __offset;
  }

  /**
   *  @brief  Inserts a number of copies of given data into the %vector.
   *  @param  __position  A const_iterator into the %vector.
   *  @param  __n  Number of elements to be inserted.
   *  @param  __x  Data to be inserted.
   *  @return  An iterator that points to the inserted data.
   *
   *  This function will insert a specified number of copies of
   *  the given data before the location specified by @a position.
   *
   *  Note that this kind of operation could be expensive for a
   *  %vector and if it is frequently used the user should
   *  consider using std::list.
   */
  iterator insert(const_iterator __position, size_type __n, const value_type &__x) {
    difference_type __offset = __position - cbegin();
    _M_fill_insert(begin() + __offset, __n, __x);
    return begin() + __offset;
  }

  /**
   *  @brief  Inserts a range into the %vector.
   *  @param  __position  A const_iterator into the %vector.
   *  @param  __first  An input iterator.
   *  @param  __last   An input iterator.
   *  @return  An iterator that points to the inserted data.
   *
   *  This function will insert copies of the data in the range
   *  [__first,__last) into the %vector before the location specified
   *  by @a pos.
   *
   *  Note that this kind of operation could be expensive for a
   *  %vector and if it is frequently used the user should
   *  consider using std::list.
   */
  template <typename _InputIterator, typename = std::_RequireInputIter<_InputIterator>>
  iterator insert(const_iterator __position, _InputIterator __first, _InputIterator __last) {
    difference_type __offset = __position - cbegin();
    _M_insert_dispatch(begin() + __offset, __first, __last, std::__false_type());
    return begin() + __offset;
  }

  /**
   *  @brief  Remove element at given position.
   *  @param  __position  Iterator pointing to element to be erased.
   *  @return  An iterator pointing to the next element (or end()).
   *
   *  This function will erase the element at the given position and thus
   *  shorten the %vector by one.
   *
   *  Note This operation could be expensive and if it is
   *  frequently used the user should consider using std::list.
   *  The user is also cautioned that this function only erases
   *  the element, and that if the element is itself a pointer,
   *  the pointed-to memory is not touched in any way.  Managing
   *  the pointer is the user's responsibility.
   */
  iterator erase(const_iterator __position) { return _M_erase(begin() + (__position - cbegin())); }

  /**
   *  @brief  Remove a range of elements.
   *  @param  __first  Iterator pointing to the first element to be erased.
   *  @param  __last  Iterator pointing to one past the last element to be
   *                  erased.
   *  @return  An iterator pointing to the element pointed to by @a __last
   *           prior to erasing (or end()).
   *
   *  This function will erase the elements in the range
   *  [__first,__last) and shorten the %vector accordingly.
   *
   *  Note This operation could be expensive and if it is
   *  frequently used the user should consider using std::list.
   *  The user is also cautioned that this function only erases
   *  the elements, and that if the elements themselves are
   *  pointers, the pointed-to memory is not touched in any way.
   *  Managing the pointer is the user's responsibility.
   */
  iterator erase(const_iterator __first, const_iterator __last) {
    const auto __beg = begin();
    const auto __cbeg = cbegin();
    return _M_erase(__beg + (__first - __cbeg), __beg + (__last - __cbeg));
  }

  /**
   *  @brief  Swaps data with another %vector.
   *  @param  __x  A %vector of the same element and allocator types.
   *
   *  This exchanges the elements between two vectors in constant time.
   *  (Three pointers, so it should be quite fast.)
   *  Note that the global std::swap() function is specialized such that
   *  std::swap(v1,v2) will feed to this function.
   *
   *  Whether the allocators are swapped depends on the allocator traits.
   */
  void swap(vector &__x) noexcept {
    this->_M_impl._M_swap_data(__x._M_impl);
    _Alloc_traits::_S_on_swap(_M_get_Tp_allocator(), __x._M_get_Tp_allocator());
  }

  /**
   *  Erases all the elements.  Note that this function only erases the
   *  elements, and that if the elements themselves are pointers, the
   *  pointed-to memory is not touched in any way.  Managing the pointer is
   *  the user's responsibility.
   */
  void clear() noexcept { _M_erase_at_end(this->_M_impl._M_start); }

 protected:
  /**
   *  Memory expansion handler.  Uses the member allocation function to
   *  obtain @a n bytes of memory, and then copies [first,last) into it.
   */
  template <typename _ForwardIterator>
  pointer _M_allocate_and_copy(size_type __n, _ForwardIterator __first, _ForwardIterator __last) {
    pointer __result = this->_M_allocate(__n);
    try {
      std::__uninitialized_copy_a(__first, __last, __result, _M_get_Tp_allocator());
      return __result;
    } catch (...) {
      _M_deallocate(__result, __n);
      throw;
    }
  }
  template <typename _InputIterator>
  void _M_range_initialize(_InputIterator __first, _InputIterator __last, std::input_iterator_tag) {
    try {
      for (; __first != __last; ++__first) emplace_back(*__first);

    } catch (...) {
      clear();
      throw;
    }
  }
  template <typename _ForwardIterator>
  void _M_range_initialize(_ForwardIterator __first, _ForwardIterator __last, std::forward_iterator_tag) {
    const size_type __n = std::distance(__first, __last);
    this->_M_impl._M_start = this->_M_allocate(_S_check_init_len(__n, _M_get_Tp_allocator()));
    this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __n;
    this->_M_impl._M_finish = std::__uninitialized_copy_a(__first, __last, this->_M_impl._M_start, _M_get_Tp_allocator());
  }

  void _M_fill_initialize(size_type __n, const value_type &__value) {
    this->_M_impl._M_finish = std::__uninitialized_fill_n_a(this->_M_impl._M_start, __n, __value, _M_get_Tp_allocator());
  }

  void _M_default_initialize(size_type __n) { this->_M_impl._M_finish = std::__uninitialized_default_n_a(this->_M_impl._M_start, __n, _M_get_Tp_allocator()); }

  // Internal assign functions follow.  The *_aux functions do the actual
  // assignment work for the range versions.

  // Called by the range assign to implement [23.1.1]/9

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 438. Ambiguity in the "do the right thing" clause
  template <typename _Integer>
  void _M_assign_dispatch(_Integer __n, _Integer __val, std::__true_type) {
    _M_fill_assign(__n, __val);
  }

  // Called by the range assign to implement [23.1.1]/9
  template <typename _InputIterator>
  void _M_assign_dispatch(_InputIterator __first, _InputIterator __last, std::__false_type) {
    _M_assign_aux(__first, __last, std::__iterator_category(__first));
  }

  // Called by the second assign_dispatch above
  template <typename _InputIterator>
  void _M_assign_aux(_InputIterator __first, _InputIterator __last, std::input_iterator_tag);

  // Called by the second assign_dispatch above
  template <typename _ForwardIterator>
  void _M_assign_aux(_ForwardIterator __first, _ForwardIterator __last, std::forward_iterator_tag);

  // Called by assign(n,t), and the range assign when it turns out
  // to be the same thing.
  void _M_fill_assign(size_type __n, const value_type &__val);

  // Internal insert functions follow.

  // Called by the range insert to implement [23.1.1]/9

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 438. Ambiguity in the "do the right thing" clause
  template <typename _Integer>
  void _M_insert_dispatch(iterator __pos, _Integer __n, _Integer __val, std::__true_type) {
    _M_fill_insert(__pos, __n, __val);
  }

  // Called by the range insert to implement [23.1.1]/9
  template <typename _InputIterator>
  void _M_insert_dispatch(iterator __pos, _InputIterator __first, _InputIterator __last, std::__false_type) {
    _M_range_insert(__pos, __first, __last, std::__iterator_category(__first));
  }

  // Called by the second insert_dispatch above
  template <typename _InputIterator>
  void _M_range_insert(iterator __pos, _InputIterator __first, _InputIterator __last, std::input_iterator_tag);

  // Called by the second insert_dispatch above
  template <typename _ForwardIterator>
  void _M_range_insert(iterator __pos, _ForwardIterator __first, _ForwardIterator __last, std::forward_iterator_tag);

  // Called by insert(p,n,x), and the range insert when it turns out to be
  // the same thing.
  void _M_fill_insert(iterator __pos, size_type __n, const value_type &__x);

  // Called by resize(n).
  void _M_default_append(size_type __n);

  bool _M_shrink_to_fit();

  // A value_type object constructed with _Alloc_traits::construct()
  // and destroyed with _Alloc_traits::destroy().
  struct _Temporary_value {
    template <typename... _Args>
    explicit _Temporary_value(vector *__vec, _Args &&...__args) : _M_this(__vec) {
      _Alloc_traits::construct(_M_this->_M_impl, _M_ptr(), std::forward<_Args>(__args)...);
    }

    ~_Temporary_value() { _Alloc_traits::destroy(_M_this->_M_impl, _M_ptr()); }

    value_type &_M_val() { return *_M_ptr(); }

   private:
    _Tp *_M_ptr() { return reinterpret_cast<_Tp *>(&__buf); }

    vector *_M_this;
    typename std::aligned_storage<sizeof(_Tp), alignof(_Tp)>::type __buf;
  };

  // Called by insert(p,x) and other functions when insertion needs to
  // reallocate or move existing elements. _Arg is either _Tp& or _Tp.
  template <typename _Arg>
  void _M_insert_aux(iterator __position, _Arg &&__arg);

  template <typename... _Args>
  void _M_realloc_insert(iterator __position, _Args &&...__args);

  // Either move-construct at the end, or forward to _M_insert_aux.
  iterator _M_insert_rval(const_iterator __position, value_type &&__v);

  // Try to emplace at the end, otherwise forward to _M_insert_aux.
  template <typename... _Args>
  iterator _M_emplace_aux(const_iterator __position, _Args &&...__args);

  // Emplacing an rvalue of the correct type can use _M_insert_rval.
  iterator _M_emplace_aux(const_iterator __position, value_type &&__v) { return _M_insert_rval(__position, std::move(__v)); }

  // Called by _M_fill_insert, _M_insert_aux etc.
  size_type _M_check_len(size_type __n, const char *__s) const {
    if (max_size() - size() < __n) std::__throw_length_error((__s));

    const size_type __len = size() + (std::max)(size(), __n);
    return (__len < size() || __len > max_size()) ? max_size() : __len;
  }

  // Called by constructors to check initial size.
  static size_type _S_check_init_len(size_type __n, const allocator_type &__a) {
    if (__n > _S_max_size(_Tp_alloc_type(__a))) std::__throw_length_error(("cannot create std::vector larger than max_size()"));
    return __n;
  }

  static size_type _S_max_size(const _Tp_alloc_type &__a) noexcept {
    // std::distance(begin(), end()) cannot be greater than PTRDIFF_MAX,
    // and realistically we can't store more than PTRDIFF_MAX/sizeof(T)
    // (even if std::allocator_traits::max_size says we can).
    const size_t __diffmax = __gnu_cxx::__numeric_traits<ptrdiff_t>::__max / sizeof(_Tp);
    const size_t __allocmax = _Alloc_traits::max_size(__a);
    return (std::min)(__diffmax, __allocmax);
  }

  // Internal erase functions follow.

  // Called by erase(q1,q2), clear(), resize(), _M_fill_assign,
  // _M_assign_aux.
  void _M_erase_at_end(pointer __pos) noexcept {
    if (size_type __n = this->_M_impl._M_finish - __pos) {
      std::_Destroy(__pos, this->_M_impl._M_finish, _M_get_Tp_allocator());
      this->_M_impl._M_finish = __pos;
      ;
    }
  }

  iterator _M_erase(iterator __position);

  iterator _M_erase(iterator __first, iterator __last);

 private:
  // Constant-time move assignment when source object's memory can be
  // moved, either because the source's allocator will move too
  // or because the allocators are equal.
  void _M_move_assign(vector &&__x, std::true_type) noexcept {
    vector __tmp(get_allocator());
    // swap pointers _M_start, _M_finish, _M_end_of_storage
    this->_M_impl._M_swap_data(__x._M_impl);
    __tmp._M_impl._M_swap_data(__x._M_impl);
    std::__alloc_on_move(_M_get_Tp_allocator(), __x._M_get_Tp_allocator());
  }

  // Do move assignment when it might not be possible to move source
  // object's memory, resulting in a linear-time operation.
  void _M_move_assign(vector &&__x, std::false_type) {
    if (__x._M_get_Tp_allocator() == this->_M_get_Tp_allocator())
      _M_move_assign(std::move(__x), std::true_type());
    else {
      // The rvalue's allocator cannot be moved and is not equal,
      // so we need to individually move each element.
      this->_M_assign_aux(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()), std::random_access_iterator_tag());
      __x.clear();
    }
  }

  template <typename _Up>
  _Up *_M_data_ptr(_Up *__ptr) const noexcept {
    return __ptr;
  }

  template <typename _Ptr>
  typename std::pointer_traits<_Ptr>::element_type *_M_data_ptr(_Ptr __ptr) const {
    return empty() ? nullptr : std::__to_address(__ptr);
  }

 private:
  // implementation to use with std version of allocator
  template <typename _Al, typename = void>
  struct expander {
    static bool extend(_Al &, pointer, std::size_t, std::size_t) { return false; }
  };

  // implementation to use custom version of allocator that has
  // defined 'continuous_allocator' type in its scope
  template <typename _Al>
  // for compiler supporting C++17
  struct expander<_Al, std::void_t<typename _Al::continuous_allocator>> {
    static bool extend(_Al &a, pointer ptr, std::size_t sz, std::size_t new_sz) {
      if (size_type new_len = new_sz; a.extend_allocation(ptr, sz, new_len)) return true;
      return false;
    }
  };

  bool grow(std::size_t nmbr) {
    bool retValue = false;
    if (pointer ptr = _M_impl._M_start; ptr) {
      std::size_t sz = capacity();
      std::size_t new_sz = _M_check_len(nmbr, "vector::grow");
      if (retValue = expander<allocator_type>::extend(_M_get_Tp_allocator(), ptr, sz, new_sz); retValue) _M_impl._M_end_of_storage = ptr + new_sz;
    }
    return retValue;
  }
};

template <typename _InputIterator, typename _ValT = typename std::iterator_traits<_InputIterator>::value_type, typename _Allocator = std::allocator<_ValT>,
          typename = std::_RequireInputIter<_InputIterator>, typename = std::_RequireAllocator<_Allocator>>
vector(_InputIterator, _InputIterator, _Allocator = _Allocator()) -> vector<_ValT, _Allocator>;

/**
 *  @brief  Vector equality comparison.
 *  @param  __x  A %vector.
 *  @param  __y  A %vector of the same type as @a __x.
 *  @return  True iff the size and elements of the vectors are equal.
 *
 *  This is an equivalence relation.  It is linear in the size of the
 *  vectors.  Vectors are considered equivalent if their sizes are equal,
 *  and if corresponding elements compare equal.
 */
template <typename _Tp, typename _Alloc>
inline bool operator==(const vector<_Tp, _Alloc> &__x, const vector<_Tp, _Alloc> &__y) {
  return (__x.size() == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin()));
}

// NOTE: commented to support GCC 9.3.0 and C++17
// /**
//  *  @brief  Vector ordering relation.
//  *  @param  __x  A `vector`.
//  *  @param  __y  A `vector` of the same type as `__x`.
//  *  @return  A value indicating whether `__x` is less than, equal to,
//  *           greater than, or incomparable with `__y`.
//  *
//  *  See `std::lexicographical_compare_three_way()` for how the determination
//  *  is made. This operator is used to synthesize relational operators like
//  *  `<` and `>=` etc.
//  */
// template <typename _Tp, typename _Alloc>
// inline std::__detail::__synth3way_t<_Tp> operator<=>(const vector<_Tp, _Alloc> &__x, const vector<_Tp, _Alloc> &__y) {
//   return std::lexicographical_compare_three_way(__x.begin(), __x.end(), __y.begin(), __y.end(), std::__detail::__synth3way);
// }

/// See std::vector::swap().
template <typename _Tp, typename _Alloc>
inline void swap(vector<_Tp, _Alloc> &__x, vector<_Tp, _Alloc> &__y) noexcept(noexcept(__x.swap(__y))) {
  __x.swap(__y);
}

namespace __detail::__variant {
template <typename>
struct _Never_valueless_alt;  // see <variant>

// Provide the strong exception-safety guarantee when emplacing a
// vector into a variant, but only if move assignment cannot throw.
template <typename _Tp, typename _Alloc>
struct _Never_valueless_alt<vector<_Tp, _Alloc>> : std::is_nothrow_move_assignable<vector<_Tp, _Alloc>> {};
}  // namespace __detail::__variant

/////////////////////////////////////////////////////////////////////////////
template <typename _Tp, typename _Alloc>
void vector<_Tp, _Alloc>::reserve(size_type __n) {
  if (__n > this->max_size()) std::__throw_length_error(("vector::reserve"));
  if (this->capacity() < __n) {
    if (grow(__n))
      reserve(__n);
    else {
      const size_type __old_size = size();
      pointer __tmp;

      if constexpr (_S_use_relocate()) {
        __tmp = this->_M_allocate(__n);
        _S_relocate(this->_M_impl._M_start, this->_M_impl._M_finish, __tmp, _M_get_Tp_allocator());
      } else {
        __tmp = _M_allocate_and_copy(__n, std::__make_move_if_noexcept_iterator(this->_M_impl._M_start),
                                     std::__make_move_if_noexcept_iterator(this->_M_impl._M_finish));
        std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
      }
      _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
      this->_M_impl._M_start = __tmp;
      this->_M_impl._M_finish = __tmp + __old_size;
      this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __n;
    }
  }
}
template <typename _Tp, typename _Alloc>
template <typename... _Args>
typename vector<_Tp, _Alloc>::reference vector<_Tp, _Alloc>::emplace_back(_Args &&...__args) {
  if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage) {
    _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, std::forward<_Args>(__args)...);
    ++this->_M_impl._M_finish;
  } else if (grow(1))
    emplace_back(std::forward<_Args>(__args)...);
  else
    _M_realloc_insert(end(), std::forward<_Args>(__args)...);

  return back();
}
template <typename _Tp, typename _Alloc>
typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::insert(const_iterator __position, const value_type &__x) {
  const size_type __n = __position - begin();
  if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage)
    if (__position == end()) {
      _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, __x);
      ++this->_M_impl._M_finish;
    } else {
      const auto __pos = begin() + (__position - cbegin());
      // __x could be an existing element of this vector, so make a
      // copy of it before _M_insert_aux moves elements around.
      _Temporary_value __x_copy(this, __x);
      _M_insert_aux(__pos, std::move(__x_copy._M_val()));
    }
  else if (grow(1))
    insert(__position, __x);
  else
    _M_realloc_insert(begin() + (__position - cbegin()), __x);

  return iterator(this->_M_impl._M_start + __n);
}

template <typename _Tp, typename _Alloc>
typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::_M_erase(iterator __position) {
  if (__position + 1 != end()) std::move(__position + 1, end(), __position);
  --this->_M_impl._M_finish;
  _Alloc_traits::destroy(this->_M_impl, this->_M_impl._M_finish);
  return __position;
}

template <typename _Tp, typename _Alloc>
typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::_M_erase(iterator __first, iterator __last) {
  if (__first != __last) {
    if (__last != end()) std::move(__last, end(), __first);
    _M_erase_at_end(__first.base() + (end() - __last));
  }
  return __first;
}
template <typename _Tp, typename _Alloc>
vector<_Tp, _Alloc> &vector<_Tp, _Alloc>::operator=(const vector<_Tp, _Alloc> &__x) {
  if (&__x != this) {
    if (_Alloc_traits::_S_propagate_on_copy_assign()) {
      // call _M_get_Tp_allocator().operator=(__x._M_get_Tp_allocator())
      if (!_Alloc_traits::_S_always_equal() && _M_get_Tp_allocator() != __x._M_get_Tp_allocator()) {
        // replacement allocator cannot free existing storage, so cleanup first
        this->clear();
        _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
        this->_M_impl._M_start = nullptr;
        this->_M_impl._M_finish = nullptr;
        this->_M_impl._M_end_of_storage = nullptr;
      }
      std::__alloc_on_copy(_M_get_Tp_allocator(), __x._M_get_Tp_allocator());
    }

    // copy all elements from __x to this
    const size_type __xlen = __x.size();
    if (__xlen > capacity())
      if (grow(__xlen - capacity()))
        operator=(__x);
      else {
        pointer __tmp = _M_allocate_and_copy(__xlen, __x.begin(), __x.end());
        std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
        _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
        this->_M_impl._M_start = __tmp;
        this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __xlen;
      }
    else if (size() >= __xlen) {
      std::_Destroy(std::copy(__x.begin(), __x.end(), begin()), end(), _M_get_Tp_allocator());
    } else {
      std::copy(__x._M_impl._M_start, __x._M_impl._M_start + size(), this->_M_impl._M_start);
      std::__uninitialized_copy_a(__x._M_impl._M_start + size(), __x._M_impl._M_finish, this->_M_impl._M_finish, _M_get_Tp_allocator());
    }
    this->_M_impl._M_finish = this->_M_impl._M_start + __xlen;
  }
  return *this;
}
template <typename _Tp, typename _Alloc>
void vector<_Tp, _Alloc>::_M_fill_assign(size_t __n, const value_type &__val) {
  if (__n > capacity())
    if (grow(__n - capacity()))
      _M_fill_assign(__n, __val);
    else {
      vector __tmp(__n, __val, _M_get_Tp_allocator());
      __tmp._M_impl._M_swap_data(this->_M_impl);
    }
  else if (__n > size()) {
    std::fill(begin(), end(), __val);
    const size_type __add = __n - size();
    this->_M_impl._M_finish = std::__uninitialized_fill_n_a(this->_M_impl._M_finish, __add, __val, _M_get_Tp_allocator());
  } else
    _M_erase_at_end(std::fill_n(this->_M_impl._M_start, __n, __val));
}
template <typename _Tp, typename _Alloc>
template <typename _InputIterator>
void vector<_Tp, _Alloc>::_M_assign_aux(_InputIterator __first, _InputIterator __last, std::input_iterator_tag) {
  pointer __cur(this->_M_impl._M_start);
  for (; __first != __last && __cur != this->_M_impl._M_finish; ++__cur, (void)++__first) *__cur = *__first;
  if (__first == __last)
    _M_erase_at_end(__cur);
  else
    _M_range_insert(end(), __first, __last, std::__iterator_category(__first));
}
template <typename _Tp, typename _Alloc>
template <typename _ForwardIterator>
void vector<_Tp, _Alloc>::_M_assign_aux(_ForwardIterator __first, _ForwardIterator __last, std::forward_iterator_tag) {
  const size_type __len = std::distance(__first, __last);

  if (__len > capacity())
    if (grow(__len))
      this->_M_assign_aux(__first, __last, std::forward_iterator_tag());
    else {
      _S_check_init_len(__len, _M_get_Tp_allocator());
      pointer __tmp(_M_allocate_and_copy(__len, __first, __last));
      std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
      _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
      this->_M_impl._M_start = __tmp;
      this->_M_impl._M_finish = this->_M_impl._M_start + __len;
      this->_M_impl._M_end_of_storage = this->_M_impl._M_finish;
    }
  else if (size() >= __len)
    _M_erase_at_end(std::copy(__first, __last, this->_M_impl._M_start));
  else {
    _ForwardIterator __mid = __first;
    std::advance(__mid, size());
    std::copy(__first, __mid, this->_M_impl._M_start);
    const size_type __attribute__((__unused__)) __n = __len - size();
    this->_M_impl._M_finish = std::__uninitialized_copy_a(__mid, __last, this->_M_impl._M_finish, _M_get_Tp_allocator());
  }
}
template <typename _Tp, typename _Alloc>
auto vector<_Tp, _Alloc>::_M_insert_rval(const_iterator __position, value_type &&__v) -> iterator {
  const auto __n = __position - cbegin();
  if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage)
    if (__position == cend()) {
      _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, std::move(__v));
      ++this->_M_impl._M_finish;
    } else
      _M_insert_aux(begin() + __n, std::move(__v));
  else if (grow(1))
    _M_insert_rval(__position, std::move(__v));
  else
    _M_realloc_insert(begin() + __n, std::move(__v));

  return iterator(this->_M_impl._M_start + __n);
}
template <typename _Tp, typename _Alloc>
template <typename... _Args>
auto vector<_Tp, _Alloc>::_M_emplace_aux(const_iterator __position, _Args &&...__args) -> iterator {
  const auto __n = __position - cbegin();
  if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage)
    if (__position == cend()) {
      _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, std::forward<_Args>(__args)...);
      ++this->_M_impl._M_finish;
    } else {
      // We need to construct a temporary because something in __args...
      // could alias one of the elements of the container and so we
      // need to use it before _M_insert_aux moves elements around.
      _Temporary_value __tmp(this, std::forward<_Args>(__args)...);
      _M_insert_aux(begin() + __n, std::move(__tmp._M_val()));
    }
  else if (grow(1))
    _M_emplace_aux(__position, std::forward<_Args>(__args)...);
  else
    _M_realloc_insert(begin() + __n, std::forward<_Args>(__args)...);

  return iterator(this->_M_impl._M_start + __n);
}

template <typename _Tp, typename _Alloc>
template <typename _Arg>
void vector<_Tp, _Alloc>::_M_insert_aux(iterator __position, _Arg &&__arg) {
  _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish, std::move(*(this->_M_impl._M_finish - 1)));
  ++this->_M_impl._M_finish;
  std::move_backward(__position.base(), this->_M_impl._M_finish - 2, this->_M_impl._M_finish - 1);
  *__position = std::forward<_Arg>(__arg);
}

template <typename _Tp, typename _Alloc>
template <typename... _Args>
void vector<_Tp, _Alloc>::_M_realloc_insert(iterator __position, _Args &&...__args) {
  const size_type __len = _M_check_len(size_type(1), "vector::_M_realloc_insert");
  pointer __old_start = this->_M_impl._M_start;
  pointer __old_finish = this->_M_impl._M_finish;
  const size_type __elems_before = __position - begin();
  pointer __new_start(this->_M_allocate(__len));
  pointer __new_finish(__new_start);
  try {
    // The order of the three operations is dictated by the C++11
    // case, where the moves could alter a new element belonging
    // to the existing vector.  This is an issue only for callers
    // taking the element by lvalue ref (see last bullet of C++11
    // [res.on.arguments]).
    _Alloc_traits::construct(this->_M_impl, __new_start + __elems_before, std::forward<_Args>(__args)...);
    __new_finish = pointer();
    if constexpr (_S_use_relocate()) {
      __new_finish = _S_relocate(__old_start, __position.base(), __new_start, _M_get_Tp_allocator());
      ++__new_finish;
      __new_finish = _S_relocate(__position.base(), __old_finish, __new_finish, _M_get_Tp_allocator());
    } else {
      __new_finish = std::__uninitialized_move_if_noexcept_a(__old_start, __position.base(), __new_start, _M_get_Tp_allocator());
      ++__new_finish;
      __new_finish = std::__uninitialized_move_if_noexcept_a(__position.base(), __old_finish, __new_finish, _M_get_Tp_allocator());
    }
  } catch (...) {
    if (!__new_finish)
      _Alloc_traits::destroy(this->_M_impl, __new_start + __elems_before);
    else
      std::_Destroy(__new_start, __new_finish, _M_get_Tp_allocator());
    _M_deallocate(__new_start, __len);
    throw;
  }

  if constexpr (!_S_use_relocate()) std::_Destroy(__old_start, __old_finish, _M_get_Tp_allocator());
  _M_deallocate(__old_start, this->_M_impl._M_end_of_storage - __old_start);
  this->_M_impl._M_start = __new_start;
  this->_M_impl._M_finish = __new_finish;
  this->_M_impl._M_end_of_storage = __new_start + __len;
}
template <typename _Tp, typename _Alloc>
void vector<_Tp, _Alloc>::_M_fill_insert(iterator __position, size_type __n, const value_type &__x) {
  if (__n != 0) {
    if (size_type(this->_M_impl._M_end_of_storage - this->_M_impl._M_finish) >= __n) {
      _Temporary_value __tmp(this, __x);
      value_type &__x_copy = __tmp._M_val();
      const size_type __elems_after = end() - __position;
      pointer __old_finish(this->_M_impl._M_finish);
      if (__elems_after > __n) {
        std::__uninitialized_move_a(this->_M_impl._M_finish - __n, this->_M_impl._M_finish, this->_M_impl._M_finish, _M_get_Tp_allocator());
        this->_M_impl._M_finish += __n;
        std::move_backward(__position.base(), __old_finish - __n, __old_finish);
        std::fill(__position.base(), __position.base() + __n, __x_copy);
      } else {
        this->_M_impl._M_finish = std::__uninitialized_fill_n_a(this->_M_impl._M_finish, __n - __elems_after, __x_copy, _M_get_Tp_allocator());
        std::__uninitialized_move_a(__position.base(), __old_finish, this->_M_impl._M_finish, _M_get_Tp_allocator());
        this->_M_impl._M_finish += __elems_after;
        std::fill(__position.base(), __old_finish, __x_copy);
      }
    } else if (grow(__n))
      _M_fill_insert(__position, __n, __x);
    else {
      const size_type __len = _M_check_len(__n, "vector::_M_fill_insert");
      const size_type __elems_before = __position - begin();
      pointer __new_start(this->_M_allocate(__len));
      pointer __new_finish(__new_start);
      try {
        // See _M_realloc_insert above.
        std::__uninitialized_fill_n_a(__new_start + __elems_before, __n, __x, _M_get_Tp_allocator());
        __new_finish = pointer();
        __new_finish = std::__uninitialized_move_if_noexcept_a(this->_M_impl._M_start, __position.base(), __new_start, _M_get_Tp_allocator());
        __new_finish += __n;
        __new_finish = std::__uninitialized_move_if_noexcept_a(__position.base(), this->_M_impl._M_finish, __new_finish, _M_get_Tp_allocator());
      } catch (...) {
        if (!__new_finish)
          std::_Destroy(__new_start + __elems_before, __new_start + __elems_before + __n, _M_get_Tp_allocator());
        else
          std::_Destroy(__new_start, __new_finish, _M_get_Tp_allocator());
        _M_deallocate(__new_start, __len);
        throw;
      }
      std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
      _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
      this->_M_impl._M_start = __new_start;
      this->_M_impl._M_finish = __new_finish;
      this->_M_impl._M_end_of_storage = __new_start + __len;
    }
  }
}
template <typename _Tp, typename _Alloc>
void vector<_Tp, _Alloc>::_M_default_append(size_type __n) {
  if (__n != 0) {
    const size_type __size = size();
    size_type __navail = size_type(this->_M_impl._M_end_of_storage - this->_M_impl._M_finish);

    if (__size > max_size() || __navail > max_size() - __size) __builtin_unreachable();

    if (__navail >= __n) {
      this->_M_impl._M_finish = std::__uninitialized_default_n_a(this->_M_impl._M_finish, __n, _M_get_Tp_allocator());
    } else if (grow(__n))
      _M_default_append(__n);
    else {
      const size_type __len = _M_check_len(__n, "vector::_M_default_append");
      pointer __new_start(this->_M_allocate(__len));
      if constexpr (_S_use_relocate()) {
        try {
          std::__uninitialized_default_n_a(__new_start + __size, __n, _M_get_Tp_allocator());
        } catch (...) {
          _M_deallocate(__new_start, __len);
          throw;
        }
        _S_relocate(this->_M_impl._M_start, this->_M_impl._M_finish, __new_start, _M_get_Tp_allocator());
      } else {
        pointer __destroy_from = pointer();
        try {
          std::__uninitialized_default_n_a(__new_start + __size, __n, _M_get_Tp_allocator());
          __destroy_from = __new_start + __size;
          std::__uninitialized_move_if_noexcept_a(this->_M_impl._M_start, this->_M_impl._M_finish, __new_start, _M_get_Tp_allocator());
        } catch (...) {
          if (__destroy_from) std::_Destroy(__destroy_from, __destroy_from + __n, _M_get_Tp_allocator());
          _M_deallocate(__new_start, __len);
          throw;
        }
        std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
      }
      _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
      this->_M_impl._M_start = __new_start;
      this->_M_impl._M_finish = __new_start + __size + __n;
      this->_M_impl._M_end_of_storage = __new_start + __len;
    }
  }
}

template <typename _Tp, typename _Alloc>
bool vector<_Tp, _Alloc>::_M_shrink_to_fit() {
  if (capacity() == size()) return false;
  return std::__shrink_to_fit_aux<vector>::_S_do_it(*this);
}

template <typename _Tp, typename _Alloc>
template <typename _InputIterator>
void vector<_Tp, _Alloc>::_M_range_insert(iterator __pos, _InputIterator __first, _InputIterator __last, std::input_iterator_tag) {
  if (__pos == end()) {
    for (; __first != __last; ++__first) insert(end(), *__first);
  } else if (__first != __last) {
    vector __tmp(__first, __last, _M_get_Tp_allocator());
    insert(__pos, std::make_move_iterator(__tmp.begin()), std::make_move_iterator(__tmp.end()));
  }
}

template <typename _Tp, typename _Alloc>
template <typename _ForwardIterator>
void vector<_Tp, _Alloc>::_M_range_insert(iterator __position, _ForwardIterator __first, _ForwardIterator __last, std::forward_iterator_tag) {
  if (__first != __last) {
    const size_type __n = std::distance(__first, __last);
    if (size_type(this->_M_impl._M_end_of_storage - this->_M_impl._M_finish) >= __n) {
      const size_type __elems_after = end() - __position;
      pointer __old_finish(this->_M_impl._M_finish);
      if (__elems_after > __n) {
        std::__uninitialized_move_a(this->_M_impl._M_finish - __n, this->_M_impl._M_finish, this->_M_impl._M_finish, _M_get_Tp_allocator());
        this->_M_impl._M_finish += __n;
        std::move_backward(__position.base(), __old_finish - __n, __old_finish);
        std::copy(__first, __last, __position);
      } else {
        _ForwardIterator __mid = __first;
        std::advance(__mid, __elems_after);
        std::__uninitialized_copy_a(__mid, __last, this->_M_impl._M_finish, _M_get_Tp_allocator());
        this->_M_impl._M_finish += __n - __elems_after;
        std::__uninitialized_move_a(__position.base(), __old_finish, this->_M_impl._M_finish, _M_get_Tp_allocator());
        this->_M_impl._M_finish += __elems_after;
        std::copy(__first, __mid, __position);
      }
    } else if (grow(__n))
      _M_range_insert(__position, __first, __last, std::forward_iterator_tag());
    else {
      const size_type __len = _M_check_len(__n, "vector::_M_range_insert");
      pointer __new_start(this->_M_allocate(__len));
      pointer __new_finish(__new_start);
      try {
        __new_finish = std::__uninitialized_move_if_noexcept_a(this->_M_impl._M_start, __position.base(), __new_start, _M_get_Tp_allocator());
        __new_finish = std::__uninitialized_copy_a(__first, __last, __new_finish, _M_get_Tp_allocator());
        __new_finish = std::__uninitialized_move_if_noexcept_a(__position.base(), this->_M_impl._M_finish, __new_finish, _M_get_Tp_allocator());
      } catch (...) {
        std::_Destroy(__new_start, __new_finish, _M_get_Tp_allocator());
        _M_deallocate(__new_start, __len);
        throw;
      }
      std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish, _M_get_Tp_allocator());
      _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);
      this->_M_impl._M_start = __new_start;
      this->_M_impl._M_finish = __new_finish;
      this->_M_impl._M_end_of_storage = __new_start + __len;
    }
  }
}

// NOTE: vector<bool> IS NOT SUPPORTED

}  // namespace ak_container

#endif
