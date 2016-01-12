#serial 24

dnl Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2006 Free
dnl Software Foundation, Inc.
dnl
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

AC_DEFUN([gl_REGEX],
[
  gl_INCLUDED_REGEX([lib/regex.c])
])

dnl Usage: gl_INCLUDED_REGEX([lib/regex.c])
dnl
AC_DEFUN([gl_INCLUDED_REGEX],
  [
    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    # Assume we'll default to using the included regex.c.
    ac_use_included_regex=yes

    # However, if the system regex support is good enough that it passes the
    # the following run test, then default to *not* using the included regex.c.
    # If cross compiling, assume the test would fail and use the included
    # regex.c.  The first failing regular expression is from `Spencer ere
    # test #75' in grep-2.3.
    AC_CACHE_CHECK([for working re_compile_pattern],
		   jm_cv_func_working_re_compile_pattern,
      [AC_TRY_RUN(
[#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
	  int
	  main ()
	  {
	    static struct re_pattern_buffer regex;
	    const char *s;
	    struct re_registers regs;
	    re_set_syntax (RE_SYNTAX_POSIX_EGREP);
	    memset (&regex, 0, sizeof (regex));
	    [s = re_compile_pattern ("a[[:@:>@:]]b\n", 9, &regex);]
	    /* This should fail with _Invalid character class name_ error.  */
	    if (!s)
	      exit (1);

	    /* This should succeed, but doesn't for e.g. glibc-2.1.3.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("{1", 2, &regex);

	    if (s)
	      exit (1);

	    /* The following example is derived from a problem report
               against gawk from Jorge Stolfi <stolfi@ic.unicamp.br>.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("[[an\371]]*n", 7, &regex);
	    if (s)
	      exit (1);

	    /* This should match, but doesn't for e.g. glibc-2.2.1.  */
	    if (re_match (&regex, "an", 2, 0, &regs) != 2)
	      exit (1);

	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("x", 1, &regex);
	    if (s)
	      exit (1);

	    /* The version of regex.c in e.g. GNU libc-2.2.93 didn't
	       work with a negative RANGE argument.  */
	    if (re_search (&regex, "wxy", 3, 2, -2, &regs) != 1)
	      exit (1);

	    exit (0);
	  }
	],
	       jm_cv_func_working_re_compile_pattern=yes,
	       jm_cv_func_working_re_compile_pattern=no,
	       dnl When crosscompiling, assume it's broken.
	       jm_cv_func_working_re_compile_pattern=no)])
    if test $jm_cv_func_working_re_compile_pattern = yes; then
      ac_use_included_regex=no
    fi

    test -n "$1" || AC_MSG_ERROR([missing argument])
    m4_syscmd([test -f $1])
    ifelse(m4_sysval, 0,
      [
	AC_ARG_WITH(included-regex,
	[  --without-included-regex don't compile regex; this is the default on
                          systems with version 2 of the GNU C library
                          (use with caution on other system)],
		    jm_with_regex=$withval,
		    jm_with_regex=$ac_use_included_regex)
	if test "$jm_with_regex" = yes; then
	  AC_LIBOBJ(regex)
	  gl_PREREQ_REGEX
	fi
      ],
    )
  ]
)

# Prerequisites of lib/regex.c.
AC_DEFUN([gl_PREREQ_REGEX],
[
  dnl FIXME: Maybe provide a btowc replacement someday: Solaris 2.5.1 lacks it.
  dnl FIXME: Check for wctype and iswctype, and and add -lw if necessary
  dnl to get them.

  dnl Persuade glibc <string.h> to declare mempcpy().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS_ONCE(wchar.h wctype.h)
  AC_CHECK_FUNCS_ONCE(isascii mempcpy)
  AC_CHECK_FUNCS(btowc)
])
