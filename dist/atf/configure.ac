dnl
dnl Automated Testing Framework (atf)
dnl
dnl Copyright (c) 2007 The NetBSD Foundation, Inc.
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl 3. All advertising materials mentioning features or use of this
dnl    software must display the following acknowledgement:
dnl        This product includes software developed by the NetBSD
dnl        Foundation, Inc. and its contributors.
dnl 4. Neither the name of The NetBSD Foundation nor the names of its
dnl    contributors may be used to endorse or promote products derived
dnl    from this software without specific prior written permission.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
dnl CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
dnl INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
dnl MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
dnl IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
dnl DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
dnl GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
dnl INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
dnl IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
dnl OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
dnl IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl

dnl -----------------------------------------------------------------------
dnl Initialize the GNU build system.
dnl -----------------------------------------------------------------------

AC_INIT([Automated Testing Framework], [0.3], [atf-devel@NetBSD.org], [atf])
AC_PREREQ([2.60])
AC_COPYRIGHT([Copyright (c) 2007 The NetBSD Foundation, Inc.])
AC_DEFINE(PACKAGE_COPYRIGHT,
          ["Copyright (c) 2007 The NetBSD Foundation, Inc."],
          [Define to the copyright string applicable to this package.])
AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([admin])
AC_CONFIG_SRCDIR([atf.hpp])
AC_CONFIG_TESTDIR([tests/bootstrap])

AM_INIT_AUTOMAKE([1.9 check-news subdir-objects -Wall])

dnl This is just a hack to reverse the meaning of the --enable-shared
dnl flag provided by libtool.  We want it to default to building static
dnl libraries only for now, but these are painful during development.
dnl It is nice to have an easy way to enable shared library builds.
AC_ARG_ENABLE(unstable-shared,
    AS_HELP_STRING([--enable-unstable-shared],
        [Enables the build of shared libraries with unstable ABIs/APIs]),
    [case $enableval in
     yes|no) enable_shared=${enableval} ;;
     *) enable_shared=${enableval} ;;
     esac],
    [enable_shared=no])

AC_PROG_LIBTOOL

dnl -----------------------------------------------------------------------
dnl Check for the C++ compiler and required features.
dnl -----------------------------------------------------------------------

AC_LANG(C++)
AC_PROG_CXX

AC_MSG_CHECKING(whether basename takes a constant pointer)
AC_COMPILE_IFELSE(
    [#include <libgen.h>
     int main(void) {
         const char* s = "/foo/bar/";
         (void)::basename(s);
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_CONST_BASENAME], [1],
              [Define to 1 if basename takes a constant pointer]),
    AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether dirname takes a constant pointer)
AC_COMPILE_IFELSE(
    [#include <libgen.h>
     int main(void) {
         const char* s = "/foo/bar/";
         (void)::dirname(s);
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_CONST_DIRNAME], [1],
              [Define to 1 if dirname takes a constant pointer]),
    AC_MSG_RESULT(no))

AC_CHECK_FUNCS([putenv setenv unsetenv])

AC_MSG_CHECKING(whether putenv is in std)
AC_COMPILE_IFELSE(
    [#include <cstdlib>
     int main(void) {
         std::putenv("a=b");
         return 0;
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_PUTENV_IN_STD], [1],
              [Define to 1 if putenv is in std]),
    AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether setenv is in std)
AC_COMPILE_IFELSE(
    [#include <cstdlib>
     int main(void) {
         std::setenv("a", "b");
         return 0;
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_SETENV_IN_STD], [1],
              [Define to 1 if setenv is in std]),
    AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether snprintf is in std)
AC_COMPILE_IFELSE(
    [#include <cstdio>
     int main(void) {
         char buf;
         std::snprintf(&buf, 1, "");
         return 0;
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_SNPRINTF_IN_STD], [1],
              [Define to 1 if snprintf is in std]),
    AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether unsetenv is in std)
AC_COMPILE_IFELSE(
    [#include <cstdlib>
     int main(void) {
         std::unsetenv("a");
         return 0;
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_UNSETENV_IN_STD], [1],
              [Define to 1 if unsetenv is in std]),
    AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether vsnprintf is in std)
AC_COMPILE_IFELSE(
    [#include <cstdarg>
     #include <cstdio>
     int main(void) {
         va_list ap;
         char* buf = NULL;
         const char* fmt = NULL;
         std::vsnprintf(buf, 0, fmt, ap);
     }],
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_VSNPRINTF_IN_STD], [1],
              [Define to 1 if vnsprintf is in std]),
    AC_MSG_RESULT(no))

AC_CHECK_FUNCS([unmount])

dnl -----------------------------------------------------------------------
dnl User-customizable variables.
dnl -----------------------------------------------------------------------

AC_ARG_VAR([ATF_CONFSUBDIR],
           [Subdirectory of sysconfdir under which to look for files])
if test x"${ATF_CONFSUBDIR-unset}" = x"unset"; then
    ATF_CONFSUBDIR=atf
else
    case ${ATF_CONFSUBDIR} in
    /*)
        AC_MSG_ERROR([ATF_CONFSUBDIR must hold a relative path])
        ;;
    *)
        ;;
    esac
fi
if test x"${ATF_CONFSUBDIR}" = x""; then
    AC_SUBST(atf_confdir, \${sysconfdir})
else
    AC_SUBST(atf_confdir, \${sysconfdir}/${ATF_CONFSUBDIR})
fi

AC_ARG_VAR([ATF_WORKDIR],
           [Default location to use for ATF work directories])
if test x"${ATF_WORKDIR}" = x""; then
    for t in /tmp /var/tmp; do
        if test -d ${t}; then
            ATF_WORKDIR=${t}
            break
        fi
    done
    if test x"${ATF_WORKDIR}" = x""; then
        AC_MSG_ERROR([Could not guess a value for ATF_WORKDIR])
    fi
else
    case ${ATF_WORKDIR} in
    /*)
        ;;
    *)
        AC_MSG_ERROR([ATF_WORKDIR must hold an absolute path])
        ;;
    esac
fi

AC_SUBST(atf_cssdir, \${datadir}/examples/atf)
AC_SUBST(atf_dtddir, \${datadir}/xml/atf)
AC_SUBST(atf_egdir, \${datadir}/examples/atf)
AC_SUBST(atf_xsldir, \${datadir}/xsl/atf)

dnl -----------------------------------------------------------------------
dnl Check for the shell and portability problems.
dnl -----------------------------------------------------------------------

AC_ARG_VAR([ATF_SHELL], [Location of the POSIX shell interpreter to use])
if test x"${ATF_SHELL}" = x""; then
    AC_PATH_PROGS(ATF_SHELL, [bash sh])
else
    case ${ATF_SHELL} in
    /*)
        ;;
    *)
        AC_MSG_ERROR([ATF_SHELL must hold an absolute path])
        ;;
    esac
fi
if test x"${ATF_SHELL}" = x""; then
    AC_MSG_ERROR([No POSIX shell interpreter found; maybe set ATF_SHELL?])
fi

AC_MSG_CHECKING([whether ${ATF_SHELL} supports trap -])
output=`${ATF_SHELL} -c 'trap -- -' 2>&1`
if test x"${output}" != x""; then
    AC_SUBST([SH_TRAP_DASH], [no])
    AC_MSG_RESULT([no])
else
    AC_SUBST([SH_TRAP_DASH], [yes])
    AC_MSG_RESULT([yes])
fi

dnl -----------------------------------------------------------------------
dnl Check for required tools.
dnl -----------------------------------------------------------------------

AC_PATH_PROG([MTN], [mtn])

dnl -----------------------------------------------------------------------
dnl Set up the developer mode
dnl -----------------------------------------------------------------------

AC_ARG_ENABLE(developer,
              AS_HELP_STRING(--enable-developer,
                             [enable developer features]),,
              [case ${PACKAGE_VERSION} in
               0.*|*99*|*alpha*|*beta*)
                   enable_developer=yes
                   ;;
               *)
                   enable_developer=no
                   ;;
               esac])

if test ${enable_developer} = yes; then
    try_flags="-g \
               -Wabi \
               -Wall \
               -Wcast-qual \
               -Wctor-dtor-privacy \
               -Werror \
               -Wextra \
               -Wno-deprecated \
               -Wno-non-template-friend \
               -Wno-pmf-conversions \
               -Wno-sign-compare \
               -Wno-unused-parameter \
               -Wnon-virtual-dtor \
               -Woverloaded-virtual \
               -Wpointer-arith \
               -Wredundant-decls \
               -Wreorder \
               -Wreturn-type \
               -Wshadow \
               -Wsign-promo \
               -Wswitch \
               -Wsynth \
               -Wwrite-strings"
    #
    # The following flags should also be enabled but cannot be.  Reasons
    # given below.
    #
    # -Wold-style-cast: Raises errors when using TIOCGWINSZ, at least under
    #                   Mac OS X.  This is due to the way _IOR is defined.
    #
else
    try_flags="-DNDEBUG"
fi

valid=""
for f in ${try_flags}
do
    AC_MSG_CHECKING(whether ${CXX} supports ${f})
    saved_cxxflags="${CXXFLAGS}"
    CXXFLAGS="${CXXFLAGS} ${f}"
    AC_COMPILE_IFELSE([int main(void) { return 0; }],
                      AC_MSG_RESULT(yes)
                      valid="${valid} ${f}",
                      AC_MSG_RESULT(no))
    CXXFLAGS="${saved_cxxflags}"
done
CXXFLAGS="${CXXFLAGS} ${valid}"

dnl -----------------------------------------------------------------------
dnl Finally, generate output.
dnl -----------------------------------------------------------------------

AC_OUTPUT([Makefile subrs/atf.config.subr])

if test ${enable_shared} = yes; then
    AC_MSG_WARN([Shared libraries with unstable ABIs/APIs have been enabled.])
    AC_MSG_WARN([Please do not install them as part of a binary package.])
fi

dnl vim: syntax=m4:expandtab:shiftwidth=4:softtabstop=4
