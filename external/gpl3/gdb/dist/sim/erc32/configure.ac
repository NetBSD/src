dnl Copyright (C) 1996-2016 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

# In the Cygwin environment, we need some additional flags.
AC_CACHE_CHECK([for cygwin], sim_cv_os_cygwin,
[AC_EGREP_CPP(lose, [
#ifdef __CYGWIN__
lose
#endif],[sim_cv_os_cygwin=yes],[sim_cv_os_cygwin=no])])

if test x$sim_cv_os_cygwin = xyes; then
  TERMCAP='`if test -r ../../libtermcap/libtermcap.a; then echo ../../libtermcap/libtermcap.a; else echo -ltermcap; fi` -luser32'
else
  # Keep in sync with gdb's configure.ac list.
  AC_SEARCH_LIBS(tgetent, [termcap tinfo curses ncurses],
    [TERMCAP=$ac_cv_search_tgetent], [TERMCAP=""])
fi
AC_SUBST(TERMCAP)

# We prefer the in-tree readline.  Top-level dependencies make sure
# src/readline (if it's there) is configured before src/sim.
if test -r ../../readline/Makefile; then
  READLINE=../../readline/libreadline.a
else
  AC_CHECK_LIB(readline, readline, READLINE=-lreadline,
	       AC_ERROR([the required "readline" library is missing]), $TERMCAP)
fi
AC_SUBST(READLINE)
SIM_AC_OUTPUT
