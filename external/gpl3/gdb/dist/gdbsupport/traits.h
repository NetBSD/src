/* Copyright (C) 2017-2025 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDBSUPPORT_TRAITS_H
#define GDBSUPPORT_TRAITS_H

#include <type_traits>

namespace gdb {

/* Implementation of the detection idiom:

   - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4502.pdf
   - http://en.cppreference.com/w/cpp/experimental/is_detected

*/

struct nonesuch
{
  nonesuch () = delete;
  ~nonesuch () = delete;
  nonesuch (const nonesuch &) = delete;
  void operator= (const nonesuch &) = delete;
};

namespace detection_detail {
/* Implementation of the detection idiom (negative case).  */
template<typename Default, typename AlwaysVoid,
	 template<typename...> class Op, typename... Args>
struct detector
{
  using value_t = std::false_type;
  using type = Default;
};

/* Implementation of the detection idiom (positive case).  */
template<typename Default, template<typename...> class Op, typename... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
  using value_t = std::true_type;
  using type = Op<Args...>;
};

/* Detect whether Op<Args...> is a valid type, use Default if not.  */
template<typename Default, template<typename...> class Op,
	 typename... Args>
using detected_or = detector<Default, void, Op, Args...>;

/* Op<Args...> if that is a valid type, otherwise Default.  */
template<typename Default, template<typename...> class Op,
	 typename... Args>
using detected_or_t
  = typename detected_or<Default, Op, Args...>::type;

} /* detection_detail */

template<template<typename...> class Op, typename... Args>
using is_detected
  = typename detection_detail::detector<nonesuch, void, Op, Args...>::value_t;

template<template<typename...> class Op, typename... Args>
using detected_t
  = typename detection_detail::detector<nonesuch, void, Op, Args...>::type;

template<typename Default, template<typename...> class Op, typename... Args>
using detected_or = detection_detail::detected_or<Default, Op, Args...>;

template<typename Default, template<typename...> class Op, typename... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

template<typename Expected, template<typename...> class Op, typename... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template<typename To, template<typename...> class Op, typename... Args>
using is_detected_convertible
  = std::is_convertible<detected_t<Op, Args...>, To>;

/* A few trait helpers -- standard traits but with slightly nicer
   names.  Uppercase because "and/or", etc. are reserved keywords.  */

template<typename Predicate>
using Not = std::negation<Predicate>;

template<typename ...T>
using Or = std::disjunction<T...>;

template<typename ...T>
using And = std::conjunction<T...>;

/* Concepts-light-like helper to make SFINAE logic easier to read.  */
template<typename Condition>
using Requires = typename std::enable_if<Condition::value, void>::type;
}

template<typename T>
using RequireLongest = gdb::Requires<gdb::Or<std::is_same<T, LONGEST>,
					     std::is_same<T, ULONGEST>>>;

#endif /* GDBSUPPORT_TRAITS_H */
