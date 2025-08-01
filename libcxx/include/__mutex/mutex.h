//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MUTEX_MUTEX_H
#define _LIBCPP___MUTEX_MUTEX_H

#include <__config>
#include <__thread/support.h>
#include <__type_traits/is_nothrow_constructible.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

class _LIBCPP_EXPORTED_FROM_ABI _LIBCPP_CAPABILITY("mutex") mutex {
  __libcpp_mutex_t __m_ = _LIBCPP_MUTEX_INITIALIZER;

public:
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR mutex() = default;

  mutex(const mutex&)            = delete;
  mutex& operator=(const mutex&) = delete;

#  if _LIBCPP_HAS_TRIVIAL_MUTEX_DESTRUCTION
  _LIBCPP_HIDE_FROM_ABI ~mutex() = default;
#  else
  ~mutex() _NOEXCEPT;
#  endif

  _LIBCPP_ACQUIRE_CAPABILITY() void lock();
  _LIBCPP_TRY_ACQUIRE_CAPABILITY(true) bool try_lock() _NOEXCEPT;
  _LIBCPP_RELEASE_CAPABILITY void unlock() _NOEXCEPT;

  typedef __libcpp_mutex_t* native_handle_type;
  _LIBCPP_HIDE_FROM_ABI native_handle_type native_handle() { return &__m_; }
};

static_assert(is_nothrow_default_constructible<mutex>::value, "the default constructor for std::mutex must be nothrow");

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_THREADS

#endif // _LIBCPP___MUTEX_MUTEX_H
