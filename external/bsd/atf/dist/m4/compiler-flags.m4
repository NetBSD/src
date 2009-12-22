dnl
dnl Automated Testing Framework (atf)
dnl
dnl Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
dnl Check for C/C++ compiler flags
dnl -----------------------------------------------------------------------

dnl
dnl ATF_CC_FLAG(flag_name, accum_var)
dnl
dnl Checks whether the C compiler supports the flag 'flag_name' and, if
dnl found, appends it to the variable 'accum_var'.
dnl
AC_DEFUN([ATF_CC_FLAG], [
    AC_LANG_PUSH(C)
    AC_MSG_CHECKING(whether ${CC} supports $1)
    saved_cflags="${CFLAGS}"
    valid_cflag=no
    CFLAGS="${CFLAGS} $1"
    AC_LINK_IFELSE([int main(void) { return 0; }],
                   AC_MSG_RESULT(yes)
                   valid_cflag=yes,
                   AC_MSG_RESULT(no))
    CFLAGS="${saved_cflags}"
    AC_LANG_POP()

    if test ${valid_cflag} = yes; then
        $2="${$2} $1"
    fi
])

dnl
dnl ATF_CC_FLAGS(flag_names, accum_var)
dnl Checks whether the C compiler supports the flags 'flag_names', one by
dnl one, and appends the valid ones to 'accum_var'.
dnl
AC_DEFUN([ATF_CC_FLAGS], [
    valid_cflags=
    for f in $1; do
        ATF_CC_FLAG(${f}, valid_cflags)
    done
    if test -n "${valid_cflags}"; then
        $2="${$2} ${valid_cflags}"
    fi
])

dnl
dnl ATF_CXX_FLAG(flag_name, accum_var)
dnl
dnl Checks whether the C++ compiler supports the flag 'flag_name' and, if
dnl found, appends it to the variable 'accum_var'.
dnl
AC_DEFUN([ATF_CXX_FLAG], [
    AC_LANG_PUSH(C++)
    AC_MSG_CHECKING(whether ${CXX} supports $1)
    saved_cxxflags="${CXXFLAGS}"
    valid_cxxflag=no
    CXXFLAGS="${CXXFLAGS} $1"
    AC_LINK_IFELSE([int main(void) { return 0; }],
                   AC_MSG_RESULT(yes)
                   valid_cxxflag=yes,
                   AC_MSG_RESULT(no))
    CXXFLAGS="${saved_cxxflags}"
    AC_LANG_POP()

    if test ${valid_cxxflag} = yes; then
        $2="${$2} $1"
    fi
])

dnl
dnl ATF_CXX_FLAGS(flag_names, accum_var)
dnl Checks whether the C++ compiler supports the flags 'flag_names', one by
dnl one, and appends the valid ones to 'accum_var'.
dnl
AC_DEFUN([ATF_CXX_FLAGS], [
    valid_cxxflags=
    for f in $1; do
        ATF_CXX_FLAG(${f}, valid_cxxflags)
    done
    if test -n "${valid_cxxflags}"; then
        $2="${$2} ${valid_cxxflags}"
    fi
])
