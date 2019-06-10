/* Self tests for function_view for GDB, the GNU debugger.

   Copyright (C) 2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "selftest.h"
#include "common/function-view.h"

namespace selftests {
namespace function_view {

static int
plus_one_fn_int (int val)
{
  return ++val;
}

static short
plus_one_fn_short (short val)
{
  return ++val;
}

static int
call_callback_int (int val, gdb::function_view <int (int)> callback)
{
  return callback (val);
}

static void
call_callback_void (int val, gdb::function_view <void (int)> callback)
{
  callback (val);
}

struct plus_one_int_func_obj
{
  int operator () (int val)
  {
    ++call_count;
    return ++val;
  }

  /* Number of times called.  */
  int call_count = 0;
};

static void
run_tests ()
{
  /* A simple lambda.  */
  auto plus_one_lambda = [] (int val) { return ++val; };

  /* A function_view that references the lambda.  */
  gdb::function_view<int (int)> plus_one_func_view (plus_one_lambda);

  /* Check calling the lambda directly.  */
  SELF_CHECK (plus_one_lambda (0) == 1);
  SELF_CHECK (plus_one_lambda (1) == 2);

  /* Check calling lambda via the view.  */
  SELF_CHECK (plus_one_func_view (2) == 3);
  SELF_CHECK (plus_one_func_view (3) == 4);

  /* Check calling a function that takes a function_view as argument,
     by value.  Pass a lambda, making sure a function_view is properly
     constructed implicitly.  */
  SELF_CHECK (call_callback_int (1, [] (int val)
    {
      return val + 2;
    }) == 3);

  /* Same, passing a named/lvalue lambda.  */
  SELF_CHECK (call_callback_int (1, plus_one_lambda) == 2);
  /* Same, passing a named/lvalue function_view (should copy).  */
  SELF_CHECK (call_callback_int (1, plus_one_func_view) == 2);

  /* Check constructing a function view over a function-object
     callable, and calling it.  */
  plus_one_int_func_obj func_obj;
  SELF_CHECK (func_obj (0) == 1);
  SELF_CHECK (call_callback_int (1, func_obj) == 2);
  /* Check that the callable was referenced, not copied.  */
  SELF_CHECK (func_obj.call_count == 2);

  /* Check constructing a function_view over a free-function callable,
     and calling it.  */
  SELF_CHECK (call_callback_int (1, plus_one_fn_int) == 2);

  /* Check calling a function with a
     compatible-but-not-exactly-the-same prototype.  */
  SELF_CHECK (call_callback_int (1, [] (short val) -> short
    {
      return val + 2;
    }) == 3);
  /* Same, but passing a function pointer.  */
  SELF_CHECK (call_callback_int (1, plus_one_fn_short) == 2);

  /* Like std::function, a function_view that expects a void return
     can reference callables with non-void return type.  The result is
     simply discarded.  Check a lambda, function object and a function
     pointer.  */
  call_callback_void (1, [] (int val) -> int
    {
      return val + 2;
    });
  call_callback_void (1, func_obj);
  call_callback_void (1, plus_one_fn_int);

  /* Check that the main ctor doesn't hijack the copy ctor.  */
  auto plus_one_func_view2 (plus_one_func_view);
  auto plus_one_func_view3 (plus_one_func_view2);
  static_assert (std::is_same<decltype (plus_one_func_view),
		 decltype (plus_one_func_view2)>::value, "");
  static_assert (std::is_same<decltype (plus_one_func_view),
		 decltype (plus_one_func_view3)>::value, "");

  SELF_CHECK (plus_one_func_view3 (1) == 2);

  /* Likewise, but propagate a NULL callable.  If this calls the main
     function_view ctor instead of the copy ctor by mistake, then
     null_func_2 ends up non-NULL (because it'd instead reference
     null_func_1 as just another callable).  */
  constexpr gdb::function_view<int (int)> null_func_view_1 = nullptr;
  constexpr auto null_func_view_2 (null_func_view_1);

  /* While at it, check whether the function_view is bound using
     various forms, op==, op!= and op bool.  */

  /* op== */
  static_assert (null_func_view_2 == nullptr, "");
  static_assert (nullptr == null_func_view_2, "");
  static_assert (null_func_view_2 == NULL, "");
  static_assert (NULL == null_func_view_2, "");

  /* op!= */
  static_assert (!(null_func_view_2 != nullptr), "");
  static_assert (!(nullptr != null_func_view_2), "");
  static_assert (!(null_func_view_2 != NULL), "");
  static_assert (!(NULL != null_func_view_2), "");

  /* op bool */
  static_assert (!null_func_view_2, "");

  /* Check the nullptr_t ctor.  */
  constexpr gdb::function_view<int (int)> check_ctor_nullptr (nullptr);
  static_assert (!check_ctor_nullptr, "");

  /* Check the nullptr_t op= */
  gdb::function_view<int (int)> check_op_eq_null (plus_one_fn_int);
  SELF_CHECK (check_op_eq_null);
  check_op_eq_null = nullptr;
  SELF_CHECK (!check_op_eq_null);
}

} /* namespace function_view */
} /* namespace selftests */

void
_initialize_function_view_selftests ()
{
  register_self_test (selftests::function_view::run_tests);
}
