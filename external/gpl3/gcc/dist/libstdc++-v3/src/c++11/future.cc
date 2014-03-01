// future -*- C++ -*-

// Copyright (C) 2009-2013 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include <future>

namespace
{
  struct future_error_category : public std::error_category
  {
    virtual const char*
    name() const noexcept
    { return "future"; }

    virtual std::string message(int __ec) const
    {
      std::string __msg;
      switch (std::future_errc(__ec))
      {
      case std::future_errc::broken_promise:
          __msg = "Broken promise";
          break;
      case std::future_errc::future_already_retrieved:
          __msg = "Future already retrieved";
          break;
      case std::future_errc::promise_already_satisfied:
          __msg = "Promise already satisfied";
          break;
      case std::future_errc::no_state:
          __msg = "No associated state";
          break;
      default:
          __msg = "Unknown error";
          break;
      }
      return __msg;
    }
  };

  const future_error_category&
  __future_category_instance() noexcept
  {
    static const future_error_category __fec{};
    return __fec;
  }
}

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  const error_category& future_category() noexcept
  { return __future_category_instance(); }

  future_error::~future_error() noexcept { }

  const char*
  future_error::what() const noexcept { return _M_code.message().c_str(); }

#if defined(_GLIBCXX_HAS_GTHREADS) && defined(_GLIBCXX_USE_C99_STDINT_TR1) \
  && (ATOMIC_INT_LOCK_FREE > 1)
  __future_base::_Result_base::_Result_base() = default;

  __future_base::_Result_base::~_Result_base() = default;

  __future_base::_State_base::~_State_base() = default;
#endif

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std
