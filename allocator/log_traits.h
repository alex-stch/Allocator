#ifndef _LOG_TRAITS_H_
#define _LOG_TRAITS_H_

#include <iostream>

struct non_log {
  // func_name == __PRETTY_FUNCTION__
  static void log_line(const char *, std::size_t * = nullptr) {}
};

struct cout_log {
  // func_name == __PRETTY_FUNCTION__
  static void log_line(const char *func_name, std::size_t *n = nullptr) {
    if (n)
      std::cout << func_name << "\t[n = " << *n << "]\n" << std::endl;
    else
      std::cout << func_name << "\n" << std::endl;
  }
};

// TODO: file logging
// TODO: printf logging

#endif