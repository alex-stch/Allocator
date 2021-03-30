#ifndef _LINUX_PAGE_ALLOCATOR_H_
#define _LINUX_PAGE_ALLOCATOR_H_

#include <bits/c++config.h>

#include "log_traits.h"

extern "C" {
#include <sys/mman.h>  // mmap, mprotect
#include <unistd.h>    // sysconf
}

namespace ak_allocator {

namespace __detail {

/**
 * NOTE: Bit array index
 *
 * One bit is for one element (object of the type _Tp):
 * so it will occupy one and more bytes.
 *
 * Search granularity in the bit-array index can be:
 * int, long, long long (for my linux 32, 64 and 64 bits).
 * So we can search/check 32 or 64 elements at same time.
 *
 *
 * uint32_t i = 0x01234567;
 * Memory representation:
 * LE: 67 45 23 01
 * BE: 01 23 45 67  (this way humans write numbers)
 * https://www.geeksforgeeks.org/little-and-big-endian-mystery/
 *
 * The value uint32_t(0x01020304): 00000001 00000010 00000011 00000100'
 * In memory (hex):  04 03 02 01   (that is: 00000100 00000011 00000010 00000001)
 *
 * In little endian machines, last byte (most significant) of binary
 * representation of the multibyte data-type (int, float, etc) is
 * stored first.
 * In big endian machines, first byte of binary representation of
 * the multibyte data-type is stored first.
 *
 */

template <typename _Tp, typename Logger>
struct mem_pool {
  using elem_type = typename std::aligned_storage<sizeof(_Tp), alignof(_Tp)>::type;

  mem_pool() = default;

  mem_pool(const mem_pool &other) noexcept
      : pages_mmaped(other.pages_mmaped), occupied_slots(other.occupied_slots), free_slots_left(other.free_slots_left), pagesize(other.pagesize) {
    Logger::log_line(__PRETTY_FUNCTION__);
    init_pool(pages_mmaped);
  }

  // Conversion copy Ctor
  template <typename _Tp1>
  explicit mem_pool(const mem_pool<_Tp1, Logger> &other) noexcept
      : pages_mmaped(other.pages_mmaped), occupied_slots(other.occupied_slots), free_slots_left(other.free_slots_left), pagesize(other.pagesize) {
    Logger::log_line(__PRETTY_FUNCTION__);
    init_pool(pages_mmaped);
  }

  explicit mem_pool(mem_pool &&rhs) noexcept
      : begin_gp(rhs.begin_gp),
        end_gp(rhs.end_gp),
        allocation_area(rhs.allocation_area),
        first_not_commited(rhs.first_not_commited),
        pages_mmaped(rhs.pages_mmaped),
        occupied_slots(rhs.occupied_slots),
        free_slots_left(rhs.free_slots_left),
        pagesize(rhs.pagesize) {
    Logger::log_line(__PRETTY_FUNCTION__);

    rhs.begin_gp = nullptr;
    rhs.end_gp = nullptr;
    rhs.allocation_area = nullptr;
    rhs.first_not_commited = nullptr;

    rhs.pages_mmaped = 0;
    rhs.occupied_slots = 0;
    rhs.free_slots_left = 0;
    rhs.pagesize = 0;
  }

  mem_pool &operator=(const mem_pool &other) {
    Logger::log_line(__PRETTY_FUNCTION__);
    deinit_pool();

    pages_mmaped = other.pages_mmaped;
    occupied_slots = other.occupied_slots;
    free_slots_left = other.free_slots_left;
    pagesize = other.pagesize;

    init_pool(pages_mmaped);
    return *this;
  }

  mem_pool &operator=(mem_pool &&rhs) {
    Logger::log_line(__PRETTY_FUNCTION__);
    begin_gp = rhs.begin_gp;
    end_gp = rhs.end_gp;
    allocation_area = rhs.allocation_area;
    first_not_commited = rhs.first_not_commited;

    pages_mmaped = rhs.pages_mmaped;
    occupied_slots = rhs.occupied_slots;
    free_slots_left = rhs.free_slots_left;
    pagesize = rhs.pagesize;

    rhs.begin_gp = nullptr;
    rhs.end_gp = nullptr;
    rhs.allocation_area = nullptr;
    rhs.first_not_commited = nullptr;

    rhs.pages_mmaped = 0;
    rhs.occupied_slots = 0;
    rhs.free_slots_left = 0;
    rhs.pagesize = 0;
    return *this;
  }

  // Start address of the first guard page (unusable area) that prepend the usable area
  void *begin_gp{nullptr};

  // Start address of the last guard page (unusable area) that follows the
  // usable area
  void *end_gp{nullptr};

  // Start address of the first element - this is the first properly aligned for _Tp address in the usable area of the buffer (the first properly aligned
  // address after begin_gp)
  elem_type *allocation_area{nullptr};

  // First reserved, but not committed page
  void *first_not_commited{nullptr};

  // Number of pages that have been obtained by the last mmap call to the system
  std::size_t pages_mmaped{0};

  // Counter of currently allocated slots in the allocation_area
  // Also the first free slot in the allocation_area, because the allocation starts from the element 0
  std::size_t occupied_slots{0};

  // Number of free slots available in the allocation_area
  std::size_t free_slots_left{0};

  // Default size of the usable for allocation area of the pool's memory (1 MB = 256 pages per 4096 bytes, just to be)
  const std::size_t allocate_pgs{10}; /* { 256 }; */

  // System page size
  int pagesize{0};

  /**
   * @brief The function checks if the @ptr belongs to its mapping
   *
   * @param ptr     pointer to check
   * @return true   the pointer belongs to the mapping of this class
   * @return false  the pointer DO NOT belongs to the mapping of this class
   */
  bool owns(void *val) {
    Logger::log_line(__PRETTY_FUNCTION__);
    bool retValue = false;
    if (val > allocation_area && val < begin_gp) retValue = true;
    return retValue;
  }

  int set_pagesize() {
    pagesize = sysconf(_SC_PAGE_SIZE);
    std::size_t log_ps = static_cast<std::size_t>(pagesize);
    Logger::log_line(__PRETTY_FUNCTION__, &log_ps);
    return pagesize;
  }

  /**
   * @brief Commit a reserved page pg
   *
   * @param pg      The page reserved by the mmap call
   * @return true   OK
   * @return false  Can't commit the page
   */
  bool commit_page(void *pg) {
    Logger::log_line(__PRETTY_FUNCTION__);
    if (reinterpret_cast<uint64_t>(pg) < (reinterpret_cast<uint64_t>(begin_gp) + pagesize) ||
        reinterpret_cast<uint64_t>(pg) > reinterpret_cast<uint64_t>(end_gp))
      return false;

    if (mprotect(pg, pagesize, PROT_READ | PROT_WRITE) == -1)
      return false;
    else {
      first_not_commited = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(pg) + pagesize);
      return true;
    }
  }

  /**
   * @brief The function commits a range of pages from first_not_commited till pg
   *
   * @param pg      an end range address (all pages before pg and pg's page must be commited)
   * @return true   OK
   * @return false  Can't commit one or more pages
   *
   * If the function fails to commit a page, then it recovers first_not_commited (set to one past last succesfully commited page).
   */
  bool checked_range_commit(void *pg) {
    Logger::log_line(__PRETTY_FUNCTION__);
    bool retVal = true;
    if (pg > first_not_commited) {
      void *page_to_commit = first_not_commited;
      while (page_to_commit < pg) {
        retVal = commit_page(page_to_commit);
        if (!retVal) break;
        page_to_commit = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(page_to_commit) + pagesize);
      }
    }
    return retVal;
  }

  /**
   * @brief This function requests and sets memory and initializes all the members
   *
   * @param pgs     number of pages to mmap
   * @return true   OK
   * @return false  failed to mmap
   */
  bool init_pool(std::size_t pgs) {
    Logger::log_line(__PRETTY_FUNCTION__, &pgs);
    bool retValue = false;
    if (!pagesize && set_pagesize() == -1) return false;
    if (begin_gp != nullptr) return false;
    if (!pgs) pgs = allocate_pgs;

    begin_gp = mmap(NULL, pgs * pagesize, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (begin_gp == MAP_FAILED)
      retValue = false;
    else {
      pages_mmaped = pgs;
      allocation_area = reinterpret_cast<elem_type *>(reinterpret_cast<uint64_t>(begin_gp) + pagesize);
      end_gp = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(begin_gp) + ((pages_mmaped - 1) * pagesize));
      first_not_commited = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(allocation_area) + pagesize);
      occupied_slots = 0;
      free_slots_left = ((pages_mmaped * pagesize) - (2 * pagesize)) / sizeof(elem_type);
      if (!commit_page(allocation_area)) {
        deinit_pool();
        return false;
      }
      retValue = true;
    }
    return retValue;
  }

  /**
   * @brief This function requests deletes the mapping and de-initializes all the members
   *
   * @return true   OK
   * @return false  failed to munmap
   */
  bool deinit_pool() {
    Logger::log_line(__PRETTY_FUNCTION__);
    bool retValue = false;
    // already deinited
    if (begin_gp == nullptr) return false;

    if (munmap(begin_gp, pages_mmaped * pagesize) != -1) {
      allocation_area = nullptr;
      begin_gp = end_gp = first_not_commited = nullptr;
      pages_mmaped = occupied_slots = free_slots_left = 0;
      retValue = true;
    }

    return retValue;
  }

  /**
   * @brief The function deletes the previously done by get_allocation allocation call
   *
   * @param nmbr    the number of elements (slots) to allocate;
   * @return  pointer to the allocated area (number @nmbr of free slots);
   *          nullptr in case when can't allocate memory for this amount of free slots.
   */
  void *get_allocation(std::size_t nmbr) {
    Logger::log_line(__PRETTY_FUNCTION__, &nmbr);
    void *retVal = nullptr;
    if (begin_gp == nullptr && !init_pool(0)) return nullptr;
    if (free_slots_left < nmbr) return nullptr;

    // calculate the address of the last byte of the last allocated slot
    void *end_byte = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(allocation_area + occupied_slots + nmbr) - 1);
    if (!checked_range_commit(end_byte))
      retVal = nullptr;
    else {
      retVal = allocation_area + occupied_slots;
      occupied_slots += nmbr;
      free_slots_left -= nmbr;
    }
    return retVal;
  }

  /**
   * @brief This function extends the already allocated area from @old_nmbr number of elements to @new_nmbr number of elements.
   *
   * @param ptr       the pointer to the first byte of the allocated area;
   * @param old_nmbr  the current number of elements (occupied slots) in the allocated area;
   * @param new_nmbr  the needed number of elements (occupied slots) in the allocated area;
   * @return true     if the area has been expanded and the current number of elements is @new_nmbr now;
   * @return false    if the area can't be expanded (there is no/not enaugh free space after the last element of the allocated area or there are elements from
   * another allocation at the end of allocated area)
   */
  bool extend_allocation(void *ptr, std::size_t old_nmbr, std::size_t new_nmbr) {
    std::size_t allocate_nmbr = new_nmbr - old_nmbr;
    Logger::log_line(__PRETTY_FUNCTION__, &allocate_nmbr);
    bool retVal = false;
    if (!ptr) return false;
    if (begin_gp == nullptr) return false;

    if (free_slots_left < allocate_nmbr) return false;

    if (reinterpret_cast<elem_type *>(ptr) + old_nmbr != allocation_area + occupied_slots) {
      // there are allocated elements between the old allocation and free slots, we can extend only the trailing allocation
      return retVal = false;
    } else {
      // calculate the address of the last byte of the last allocated slot
      void *end_byte = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(allocation_area + occupied_slots + allocate_nmbr) - 1);
      if (!checked_range_commit(end_byte))
        retVal = false;
      else {
        occupied_slots += allocate_nmbr;
        free_slots_left -= allocate_nmbr;
        retVal = true;
      }
    }
    return retVal;
  }

  /**
   * @brief The function deletes the previously done by get_allocation allocation call
   *
   * @param ptr     the pointer that has been returned by get_allocation;
   * @param nmbr    the number of elements that has been asked from get_allocation;
   * @return true   the allocated elements have been succesfully deleted;
   * @return false  can't delete elements of this allocation (ptr or nmbr or both are not correct).
   */
  bool delete_allocation(void *ptr, std::size_t nmbr) {
    Logger::log_line(__PRETTY_FUNCTION__, &nmbr);
    bool retVal = true;
    if (!ptr) return false;
    if (begin_gp == nullptr) return false;

    // TODO: PARTIALLY IMPLEMENTED

    occupied_slots -= nmbr;
    free_slots_left += nmbr;

    return retVal;
  }

  void swap(mem_pool &other) noexcept {
    Logger::log_line(__PRETTY_FUNCTION__);
    mem_pool tmp(other);
    other = std::move(*this);
    *this = std::move(tmp);
  }
};

template <typename _Tp, typename Logger>
void swap(mem_pool<_Tp, Logger> &lhs, mem_pool<_Tp, Logger> &rhs) {
  Logger::log_line(__PRETTY_FUNCTION__);
  lhs.swap(rhs);
}

/**
 * A vector allocates memory twice as big as it has currently or more if it goes to insert more then its current size. The allocation is done in one call.
 * After allocation it copies/moves its elements to the newly allocated memory and deallocates the previously allocated memory. If the vector deletes one ore
 * more elements it does not deallocate its memory. The vector holds the memory and reuse it on the next insertion. So it gets its memory continuous in one
 * chunk.
 */

}  // namespace __detail

template <typename _Tp, std::size_t _MaxObjects = 10, typename Logger = non_log>
class page_allocator {
 public:
  using value_type = _Tp;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  // The member type is_always_equal of std::allocator_traits is intendedly used for determining whether an allocator type is stateless.
  // Instances of a stateless allocator type always compare equal. Stateless allocator types are typically implemented as empty classes and suitable for empty
  // base class optimization.
  using is_always_equal = std::false_type;
  using continuous_allocator = std::true_type;

  template <typename _Tp1>
  struct rebind {
    using other = page_allocator<_Tp1, _MaxObjects, Logger>;
  };

  constexpr page_allocator() noexcept : _pool() { Logger::log_line(__PRETTY_FUNCTION__); }

  constexpr page_allocator(const page_allocator &other) noexcept : _pool(other.get_mem_pool()) { Logger::log_line(__PRETTY_FUNCTION__); }

  constexpr page_allocator(page_allocator &&rhs) noexcept : _pool(std::move(rhs.get_mem_pool())) { Logger::log_line(__PRETTY_FUNCTION__); }

  // Conversion copy Ctor
  template <typename _Tp1, std::size_t _MaxObjects1>
  constexpr page_allocator(const page_allocator<_Tp1, _MaxObjects1, Logger> &other) noexcept : _pool(other.get_mem_pool()) {
    Logger::log_line(__PRETTY_FUNCTION__);
  }

  page_allocator &operator=(const page_allocator &other) {
    Logger::log_line(__PRETTY_FUNCTION__);
    _pool = other.get_mem_pool();
    return *this;
  }

  page_allocator &operator=(page_allocator &&rhs) {
    Logger::log_line(__PRETTY_FUNCTION__);
    _pool = std::move(rhs.get_mem_pool());
    return *this;
  }

  ~page_allocator() {
    Logger::log_line(__PRETTY_FUNCTION__);
    if (_pool.begin_gp != nullptr && !_pool.deinit_pool()) std::__throw_runtime_error("page_allocator: Can't deallocate ...");
  }

  /**
   *  @brief  Allocates uninitialized storage
   *  @param  __n  the number of objects of type T to allocate storage for
   *  @param  hint pointer to a nearby memory location
   *  @return Pointer to the first element of an array of n objects of type T
   *  whose elements have not been constructed yet.
   *  @throw
   *
   *  Allocates n * sizeof(T) bytes of uninitialized storage. Then, this function creates an array of type T[n] in the storage and starts its lifetime, but does
   *  not start lifetime of any of its elements.
   *
   *  Note that __n is permitted to be 0.  The C++ standard says nothing
   *  about what the return value is when __n == 0.
   */
  [[__nodiscard__]] _Tp *allocate(size_type __n, const void * = static_cast<const void *>(0)) {
    Logger::log_line(__PRETTY_FUNCTION__, &__n);
    if (__n > this->_M_max_size()) std::__throw_bad_alloc();
    // fix for std::map: allow _MaxObjects max allocations and then shoot 'em
    // occupied_slots start counting from 0, so +1
    if (_pool.occupied_slots + 1 > this->max_size()) std::__throw_bad_alloc();

    return static_cast<_Tp *>(_pool.get_allocation(__n));
  }

  /**
   *  @brief  Deallocates the storage obtained by an earlier call to allocate().
   *  @param  __p  pointer obtained from allocate()
   *  @param  __t  number of objects earlier passed to allocate()
   *
   *  Note that __p is not permitted to be a null pointer.
   */
  void deallocate(_Tp *__p, size_type __t) {
    Logger::log_line(__PRETTY_FUNCTION__, &__t);
    _pool.delete_allocation(__p, __t);
  }

  /**
   *  @brief  Returns the largest supported allocation size
   *
   *  Returns the maximum theoretically possible value of n, for which the call allocate(n, 0) could succeed. In most implementations, this returns
   *  std::numeric_limits<size_type>::max() / sizeof(value_type).
   */
  size_type max_size() const noexcept {
    size_type retValue = objs_number ? objs_number : _M_max_size();
    Logger::log_line(__PRETTY_FUNCTION__, &retValue);
    return retValue;
  }

  // proxy function
  bool extend_allocation(_Tp *__p, std::size_t __sz, std::size_t __new_sz) {
    Logger::log_line(__PRETTY_FUNCTION__, &__new_sz);
    return _pool.extend_allocation(__p, __sz, __new_sz);
  }

  void swap(page_allocator &other) noexcept {
    Logger::log_line(__PRETTY_FUNCTION__);
    page_allocator tmp(other);
    other = std::move(*this);
    *this = std::move(tmp);
  }

  const __detail::mem_pool<_Tp, Logger> &get_mem_pool() const noexcept {
    Logger::log_line(__PRETTY_FUNCTION__);
    return _pool;
  }

 private:
  constexpr size_type _M_max_size() const noexcept {
    Logger::log_line(__PRETTY_FUNCTION__);
    /**
     * If an array is so large (greater than PTRDIFF_MAX elements, but less than SIZE_MAX bytes), that the difference between two pointers may not be
     * representable as std::ptrdiff_t, the result of subtracting two such pointers is undefined. https://en.cppreference.com/w/cpp/types/ptrdiff_t
     */
    return std::size_t(__PTRDIFF_MAX__) / sizeof(_Tp);
  }

  // Maximum number of objects to allocate (infinity if 0)
  std::size_t objs_number{_MaxObjects};

  __detail::mem_pool<_Tp, Logger> _pool;
};

template <typename _Tp, std::size_t _MaxObjects, typename Logger>
void swap(page_allocator<_Tp, _MaxObjects, Logger> &lhs, page_allocator<_Tp, _MaxObjects, Logger> &rhs) {
  Logger::log_line(__PRETTY_FUNCTION__);
  lhs.swap(rhs);
}

template <class _Tp1, class _Tp2, std::size_t _MaxObjects, typename Logger>
bool operator==(const page_allocator<_Tp1, _MaxObjects, Logger> &, const page_allocator<_Tp2, _MaxObjects, Logger> &) noexcept {
  Logger::log_line(__PRETTY_FUNCTION__);

  /*
  Add type alignment tests to check relation between alignment and size of types.
  The info is needed for non-member operator==, because in accordance to specs:
  "two allocators compares equal (a1 == a2) only if the storage allocated by the allocator a1 can be deallocated through a2".
  So the non-member operator== for the allocator uses the size of ALIGNED types to decide if it can handle allocate/deallocate for this type. For example, if we
  use move on those two allocators.
  */

  using aligned_Tp1 = typename std::aligned_storage<sizeof(_Tp1), alignof(_Tp1)>::type;
  using aligned_Tp2 = typename std::aligned_storage<sizeof(_Tp2), alignof(_Tp2)>::type;

  if (sizeof(aligned_Tp1) == sizeof(aligned_Tp2))
    return true;
  else
    return false;
}

template <class _Tp1, class _Tp2, std::size_t _MaxObjects, typename Logger>
bool operator!=(const page_allocator<_Tp1, _MaxObjects, Logger> &__a, const page_allocator<_Tp2, _MaxObjects, Logger> &__b) noexcept {
  Logger::log_line(__PRETTY_FUNCTION__);
  return !(__a == __b);
}

// Just to simplify our life
template <typename _Tp, std::size_t _MaxObjects = 10>
using p_alloc_log = ak_allocator::page_allocator<_Tp, _MaxObjects, cout_log>;

template <typename _Tp, std::size_t _MaxObjects = 10>
using p_alloc = ak_allocator::page_allocator<_Tp, _MaxObjects, non_log>;

}  // namespace ak_allocator

#endif
