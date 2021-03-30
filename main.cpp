// clang-format off

#include "allocator/page_allocator.h"
#include "container/vector.h"

// clang-format on

#include <iostream>
#include <map>
// #include <vector>

int main() {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  std::cout << std::endl;

  auto m = std::map<int, int, std::less<int>>{};
  for (int i = 0; i < 10; ++i) {
    m[i] = [i]() -> int {
      int res = 1;
      for (int j = 1; j <= i; j++) res = res * j;
      return res;
    }();
  }

  auto n = std::map<int, int, std::less<int>, ak_allocator::p_alloc<std::pair<const int, int>>>{};
  for (int i = 0; i < 10; ++i) {
    n[i] = [i]() -> int {
      int res = 1;
      for (int j = 1; j <= i; j++) res = res * j;
      return res;
    }();
  }

  for (const auto &v : m) {
    std::cout << v.first << " " << v.second << std::endl;
  }
  std::cout << std::endl;

  for (const auto &v : n) {
    std::cout << v.first << " " << v.second << std::endl;
  }
  std::cout << std::endl;

  /////////////////////////////////////////////
  ak_container::vector<int> v;
  for (int i = 0; i < 10; ++i) v.push_back(i);

  ak_container::vector<int, ak_allocator::p_alloc<int>> akv;
  for (int i = 0; i < 10; ++i) akv.push_back(i);

  std::cout << "v: ";
  for (const auto &ev : v) std::cout << ev << ' ';
  std::cout << std::endl;

  std::cout << "akv: ";
  for (const auto &akev : akv) std::cout << akev << ' ';
  std::cout << std::endl;
  /////////////////////////////////////////////

  return 0;
}
