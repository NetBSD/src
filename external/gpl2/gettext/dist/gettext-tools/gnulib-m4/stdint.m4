# stdint.m4 serial 19
dnl Copyright (C) 2001-2002, 2004-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert and Bruno Haible.
dnl Test whether <stdint.h> is supported or must be substituted.

AC_DEFUN([gl_STDINT_H],
[
  AC_PREREQ(2.59)dnl

  dnl Check for long long int and unsigned long long int.
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  if test $ac_cv_type_long_long_int = yes; then
    HAVE_LONG_LONG_INT=1
  else
    HAVE_LONG_LONG_INT=0
  fi
  AC_SUBST([HAVE_LONG_LONG_INT])
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  if test $ac_cv_type_unsigned_long_long_int = yes; then
    HAVE_UNSIGNED_LONG_LONG_INT=1
  else
    HAVE_UNSIGNED_LONG_LONG_INT=0
  fi
  AC_SUBST([HAVE_UNSIGNED_LONG_LONG_INT])

  dnl Check for <wchar.h>.
  AC_CHECK_HEADERS_ONCE([wchar.h])
  if test $ac_cv_header_wchar_h = yes; then
    HAVE_WCHAR_H=1
  else
    HAVE_WCHAR_H=0
  fi
  AC_SUBST([HAVE_WCHAR_H])

  dnl Check for <inttypes.h>.
  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_inttypes_h.
  if test $ac_cv_header_inttypes_h = yes; then
    HAVE_INTTYPES_H=1
  else
    HAVE_INTTYPES_H=0
  fi
  AC_SUBST([HAVE_INTTYPES_H])

  dnl Check for <sys/types.h>.
  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_sys_types_h.
  if test $ac_cv_header_sys_types_h = yes; then
    HAVE_SYS_TYPES_H=1
  else
    HAVE_SYS_TYPES_H=0
  fi
  AC_SUBST([HAVE_SYS_TYPES_H])

  dnl AC_INCLUDES_DEFAULT defines $ac_cv_header_stdint_h.
  if test $ac_cv_header_stdint_h = yes; then
    gl_ABSOLUTE_HEADER([stdint.h])
    ABSOLUTE_STDINT_H=\"$gl_cv_absolute_stdint_h\"
    HAVE_STDINT_H=1
  else
    ABSOLUTE_STDINT_H=\"no/such/file/stdint.h\"
    HAVE_STDINT_H=0
  fi
  AC_SUBST([ABSOLUTE_STDINT_H])
  AC_SUBST([HAVE_STDINT_H])

  dnl Now see whether we need a substitute <stdint.h>.  Use
  dnl ABSOLUTE_STDINT_H, not <stdint.h>, so that it also works during
  dnl a "config.status --recheck" if a stdint.h has been
  dnl created in the build directory.
  if test $ac_cv_header_stdint_h = yes; then
    AC_CACHE_CHECK([whether stdint.h conforms to C99],
      [gl_cv_header_working_stdint_h],
      [gl_cv_header_working_stdint_h=no
       AC_COMPILE_IFELSE([
	 AC_LANG_PROGRAM([[
#include <stddef.h>
#define __STDC_LIMIT_MACROS 1 /* to make it work also in C++ mode */
#define __STDC_CONSTANT_MACROS 1 /* to make it work also in C++ mode */
#include ABSOLUTE_STDINT_H
#ifdef INT8_MAX
int8_t a1 = INT8_MAX;
int8_t a1min = INT8_MIN;
#endif
#ifdef INT16_MAX
int16_t a2 = INT16_MAX;
int16_t a2min = INT16_MIN;
#endif
#ifdef INT32_MAX
int32_t a3 = INT32_MAX;
int32_t a3min = INT32_MIN;
#endif
#ifdef INT64_MAX
int64_t a4 = INT64_MAX;
int64_t a4min = INT64_MIN;
#endif
#ifdef UINT8_MAX
uint8_t b1 = UINT8_MAX;
#else
typedef int b1[(unsigned char) -1 != 255 ? 1 : -1];
#endif
#ifdef UINT16_MAX
uint16_t b2 = UINT16_MAX;
#endif
#ifdef UINT32_MAX
uint32_t b3 = UINT32_MAX;
#endif
#ifdef UINT64_MAX
uint64_t b4 = UINT64_MAX;
#endif
int_least8_t c1 = INT8_C (0x7f);
int_least8_t c1max = INT_LEAST8_MAX;
int_least8_t c1min = INT_LEAST8_MIN;
int_least16_t c2 = INT16_C (0x7fff);
int_least16_t c2max = INT_LEAST16_MAX;
int_least16_t c2min = INT_LEAST16_MIN;
int_least32_t c3 = INT32_C (0x7fffffff);
int_least32_t c3max = INT_LEAST32_MAX;
int_least32_t c3min = INT_LEAST32_MIN;
int_least64_t c4 = INT64_C (0x7fffffffffffffff);
int_least64_t c4max = INT_LEAST64_MAX;
int_least64_t c4min = INT_LEAST64_MIN;
uint_least8_t d1 = UINT8_C (0xff);
uint_least8_t d1max = UINT_LEAST8_MAX;
uint_least16_t d2 = UINT16_C (0xffff);
uint_least16_t d2max = UINT_LEAST16_MAX;
uint_least32_t d3 = UINT32_C (0xffffffff);
uint_least32_t d3max = UINT_LEAST32_MAX;
uint_least64_t d4 = UINT64_C (0xffffffffffffffff);
uint_least64_t d4max = UINT_LEAST64_MAX;
int_fast8_t e1 = INT_FAST8_MAX;
int_fast8_t e1min = INT_FAST8_MIN;
int_fast16_t e2 = INT_FAST16_MAX;
int_fast16_t e2min = INT_FAST16_MIN;
int_fast32_t e3 = INT_FAST32_MAX;
int_fast32_t e3min = INT_FAST32_MIN;
int_fast64_t e4 = INT_FAST64_MAX;
int_fast64_t e4min = INT_FAST64_MIN;
uint_fast8_t f1 = UINT_FAST8_MAX;
uint_fast16_t f2 = UINT_FAST16_MAX;
uint_fast32_t f3 = UINT_FAST32_MAX;
uint_fast64_t f4 = UINT_FAST64_MAX;
#ifdef INTPTR_MAX
intptr_t g = INTPTR_MAX;
intptr_t gmin = INTPTR_MIN;
#endif
#ifdef UINTPTR_MAX
uintptr_t h = UINTPTR_MAX;
#endif
intmax_t i = INTMAX_MAX;
uintmax_t j = UINTMAX_MAX;
struct s {
  int check_PTRDIFF: PTRDIFF_MIN < 0 && 0 < PTRDIFF_MAX ? 1 : -1;
  int check_SIG_ATOMIC: SIG_ATOMIC_MIN <= 0 && 0 < SIG_ATOMIC_MAX ? 1 : -1;
  int check_SIZE: 0 < SIZE_MAX ? 1 : -1;
  int check_WCHAR: WCHAR_MIN <= 0 && 0 < WCHAR_MAX ? 1 : -1;
  int check_WINT: WINT_MIN <= 0 && 0 < WINT_MAX ? 1 : -1;

  /* Detect bugs in glibc 2.4 and Solaris 10 stdint.h, among others.  */
  int check_UINT8_C:
	(-1 < UINT8_C (0)) == (-1 < (uint_least8_t) 0) ? 1 : -1;
  int check_UINT16_C:
	(-1 < UINT16_C (0)) == (-1 < (uint_least16_t) 0) ? 1 : -1;

  /* Detect bugs in OpenBSD 3.9 stdint.h.  */
#ifdef UINT8_MAX
  int check_uint8: (uint8_t) -1 == UINT8_MAX ? 1 : -1;
#endif
#ifdef UINT16_MAX
  int check_uint16: (uint16_t) -1 == UINT16_MAX ? 1 : -1;
#endif
#ifdef UINT32_MAX
  int check_uint32: (uint32_t) -1 == UINT32_MAX ? 1 : -1;
#endif
#ifdef UINT64_MAX
  int check_uint64: (uint64_t) -1 == UINT64_MAX ? 1 : -1;
#endif
  int check_uint_least8: (uint_least8_t) -1 == UINT_LEAST8_MAX ? 1 : -1;
  int check_uint_least16: (uint_least16_t) -1 == UINT_LEAST16_MAX ? 1 : -1;
  int check_uint_least32: (uint_least32_t) -1 == UINT_LEAST32_MAX ? 1 : -1;
  int check_uint_least64: (uint_least64_t) -1 == UINT_LEAST64_MAX ? 1 : -1;
  int check_uint_fast8: (uint_fast8_t) -1 == UINT_FAST8_MAX ? 1 : -1;
  int check_uint_fast16: (uint_fast16_t) -1 == UINT_FAST16_MAX ? 1 : -1;
  int check_uint_fast32: (uint_fast32_t) -1 == UINT_FAST32_MAX ? 1 : -1;
  int check_uint_fast64: (uint_fast64_t) -1 == UINT_FAST64_MAX ? 1 : -1;
  int check_uintptr: (uintptr_t) -1 == UINTPTR_MAX ? 1 : -1;
  int check_uintmax: (uintmax_t) -1 == UINTMAX_MAX ? 1 : -1;
  int check_size: (size_t) -1 == SIZE_MAX ? 1 : -1;
};
	 ]])],
         [gl_cv_header_working_stdint_h=yes])])
  fi
  if test "$gl_cv_header_working_stdint_h" != yes; then

    dnl Check for <sys/inttypes.h>, and for
    dnl <sys/bitypes.h> (used in Linux libc4 >= 4.6.7 and libc5).
    AC_CHECK_HEADERS([sys/inttypes.h sys/bitypes.h])
    if test $ac_cv_header_sys_inttypes_h = yes; then
      HAVE_SYS_INTTYPES_H=1
    else
      HAVE_SYS_INTTYPES_H=0
    fi
    AC_SUBST([HAVE_SYS_INTTYPES_H])
    if test $ac_cv_header_sys_bitypes_h = yes; then
      HAVE_SYS_BITYPES_H=1
    else
      HAVE_SYS_BITYPES_H=0
    fi
    AC_SUBST([HAVE_SYS_BITYPES_H])

    gl_STDINT_TYPE_PROPERTIES
    STDINT_H=stdint.h
  fi
  AC_SUBST(STDINT_H)
])

dnl gl_STDINT_BITSIZEOF(TYPES, INCLUDES)
dnl Determine the size of each of the given types in bits.
AC_DEFUN([gl_STDINT_BITSIZEOF],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE([BITSIZEOF_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]),
       [Define to the number of bits in type ']gltype['.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([for bit size of $gltype], [gl_cv_bitsizeof_${gltype}],
      [_AC_COMPUTE_INT([sizeof ($gltype) * CHAR_BIT], result,
	 [$2
#include <limits.h>], [result=unknown])
       eval gl_cv_bitsizeof_${gltype}=\$result
      ])
    eval result=\$gl_cv_bitsizeof_${gltype}
    if test $result = unknown; then
      dnl Use a nonempty default, because some compilers, such as IRIX 5 cc,
      dnl do a syntax check even on unused #if conditions and give an error
      dnl on valid C code like this:
      dnl   #if 0
      dnl   # if  > 32
      dnl   # endif
      dnl   #endif
      result=0
    fi
    GLTYPE=`echo "$gltype" | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    AC_DEFINE_UNQUOTED([BITSIZEOF_${GLTYPE}], [$result])
    eval BITSIZEOF_${GLTYPE}=\$result
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST([BITSIZEOF_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]))])
])

dnl gl_CHECK_TYPES_SIGNED(TYPES, INCLUDES)
dnl Determine the signedness of each of the given types.
dnl Define HAVE_SIGNED_TYPE if type is signed.
AC_DEFUN([gl_CHECK_TYPES_SIGNED],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE([HAVE_SIGNED_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]),
       [Define to 1 if ']gltype[' is a signed integer type.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([whether $gltype is signed], [gl_cv_type_${gltype}_signed],
      [AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([$2[
            int verify[2 * (($gltype) -1 < ($gltype) 0) - 1];]])],
         result=yes, result=no)
       eval gl_cv_type_${gltype}_signed=\$result
      ])
    eval result=\$gl_cv_type_${gltype}_signed
    GLTYPE=`echo $gltype | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    if test "$result" = yes; then
      AC_DEFINE_UNQUOTED([HAVE_SIGNED_${GLTYPE}], 1)
      eval HAVE_SIGNED_${GLTYPE}=1
    else
      eval HAVE_SIGNED_${GLTYPE}=0
    fi
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST([HAVE_SIGNED_]translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_]))])
])

dnl gl_INTEGER_TYPE_SUFFIX(TYPES, INCLUDES)
dnl Determine the suffix to use for integer constants of the given types.
dnl Define t_SUFFIX for each such type.
AC_DEFUN([gl_INTEGER_TYPE_SUFFIX],
[
  dnl Use a shell loop, to avoid bloating configure, and
  dnl - extra AH_TEMPLATE calls, so that autoheader knows what to put into
  dnl   config.h.in,
  dnl - extra AC_SUBST calls, so that the right substitutions are made.
  AC_FOREACH([gltype], [$1],
    [AH_TEMPLATE(translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_])[_SUFFIX],
       [Define to l, ll, u, ul, ull, etc., as suitable for
	constants of type ']gltype['.])])
  for gltype in $1 ; do
    AC_CACHE_CHECK([for $gltype integer literal suffix],
      [gl_cv_type_${gltype}_suffix],
      [eval gl_cv_type_${gltype}_suffix=no
       eval result=\$gl_cv_type_${gltype}_signed
       if test "$result" = yes; then
	 glsufu=
       else
	 glsufu=u
       fi
       for glsuf in "$glsufu" ${glsufu}l ${glsufu}ll ${glsufu}i64; do
	 case $glsuf in
	   '')  gltype1='int';;
	   l)	gltype1='long int';;
	   ll)	gltype1='long long int';;
	   i64)	gltype1='__int64';;
	   u)	gltype1='unsigned int';;
	   ul)	gltype1='unsigned long int';;
	   ull)	gltype1='unsigned long long int';;
	   ui64)gltype1='unsigned __int64';;
	 esac
	 AC_COMPILE_IFELSE(
	   [AC_LANG_PROGRAM([$2
	      extern $gltype foo;
	      extern $gltype1 foo;])],
	   [eval gl_cv_type_${gltype}_suffix=\$glsuf])
	 eval result=\$gl_cv_type_${gltype}_suffix
	 test "$result" != no && break
       done])
    GLTYPE=`echo $gltype | tr 'abcdefghijklmnopqrstuvwxyz ' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'`
    eval result=\$gl_cv_type_${gltype}_suffix
    test "$result" = no && result=
    eval ${GLTYPE}_SUFFIX=\$result
    AC_DEFINE_UNQUOTED([${GLTYPE}_SUFFIX], $result)
  done
  AC_FOREACH([gltype], [$1],
    [AC_SUBST(translit(gltype,[abcdefghijklmnopqrstuvwxyz ],[ABCDEFGHIJKLMNOPQRSTUVWXYZ_])[_SUFFIX])])
])

dnl gl_STDINT_INCLUDES
AC_DEFUN([gl_STDINT_INCLUDES],
[[
  #include <stddef.h>
  #include <signal.h>
  #if HAVE_WCHAR_H
    /* BSD/OS 4.1 has a bug: <stdio.h> and <time.h> must be included before
       <wchar.h>.  */
  # include <stdio.h>
  # include <time.h>
  # include <wchar.h>
  #endif
]])

dnl gl_STDINT_TYPE_PROPERTIES
dnl Compute HAVE_SIGNED_t, BITSIZEOF_t and t_SUFFIX, for all the types t
dnl of interest to stdint_.h.
AC_DEFUN([gl_STDINT_TYPE_PROPERTIES],
[
  gl_STDINT_BITSIZEOF([ptrdiff_t sig_atomic_t size_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
  gl_CHECK_TYPES_SIGNED([sig_atomic_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
  gl_cv_type_ptrdiff_t_signed=yes
  gl_cv_type_size_t_signed=no
  gl_INTEGER_TYPE_SUFFIX([ptrdiff_t sig_atomic_t size_t wchar_t wint_t],
    [gl_STDINT_INCLUDES])
])
