# SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Autoconf-exception-macro
# SPDX-FileCopyrightText: 2023 Michael Jeanson <mjeanson@efficios.com>
#
# SYNOPSIS
#
#   AE_CC_ATOMIC_BUILTINS([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#

#serial 1

AC_DEFUN([AE_CC_ATOMIC_BUILTINS], [
AC_REQUIRE([AC_PROG_CC])
AC_LANG_PUSH([C])

AC_CACHE_CHECK(
  [whether $CC supports atomic builtins],
  [ae_cv_cc_atomic_builtins],
  [
    AC_LINK_IFELSE([
      AC_LANG_PROGRAM([
        int x, y;
      ],[
        __atomic_store_n(&x, 0, __ATOMIC_RELAXED);
        __atomic_load_n(&x, __ATOMIC_RELAXED);
        y = __atomic_exchange_n(&x, 1, __ATOMIC_RELAXED);
        __atomic_compare_exchange_n(&x, &y, 0, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
        __atomic_add_fetch(&x, 1, __ATOMIC_RELAXED);
        __atomic_sub_fetch(&x, 1, __ATOMIC_RELAXED);
        __atomic_and_fetch(&x, 0x01, __ATOMIC_RELAXED);
        __atomic_or_fetch(&x, 0x01, __ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_RELAXED);
        __atomic_signal_fence(__ATOMIC_RELAXED);
      ])
    ],[
      ae_cv_cc_atomic_builtins=yes
    ],[
      ae_cv_cc_atomic_builtins=no
    ])
  ]
)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "x$ae_cv_cc_atomic_builtins" = "xyes"; then
  $1
  :
else
  $2
  :
fi

AC_LANG_POP
])dnl AE_CC_ATOMIC_BUILTINS
