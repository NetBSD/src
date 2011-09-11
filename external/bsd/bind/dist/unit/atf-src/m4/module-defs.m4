dnl
dnl Automated Testing Framework (atf)
dnl
dnl Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
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

AC_DEFUN([ATF_MODULE_DEFS], [
    dnl XXX This check is overly simple and should be fixed.  For example,
    dnl Sun's cc does support the noreturn attribute but CC (the C++
    dnl compiler) does not.  And in that case, CC just raises a warning
    dnl during compilation, not an error, which later breaks the
    dnl atf-c++/t_pkg_config:cxx_build check.
    AC_MSG_CHECKING(whether __attribute__((noreturn)) is supported)
    AC_RUN_IFELSE(
        [AC_LANG_PROGRAM([], [
#if ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
    return 0;
#else
    return 1;
#endif
])],
        [AC_MSG_RESULT(yes)
         value="__attribute__((noreturn))"],
        [AC_MSG_RESULT(no)
         value=""]
    )
    AC_SUBST([ATTRIBUTE_NORETURN], [${value}])
])
