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

AC_DEFUN([ATF_CHECK_IN_STD], [
    AC_MSG_CHECKING(whether $1 is in std)
    AC_COMPILE_IFELSE(
        [$2
         int main(void) {
             $3
             return 0;
         }],
        AC_MSG_RESULT(yes)
        AC_DEFINE([HAVE_]translit($1, [a-z], [A-Z])[_IN_STD], [1],
                  [Define to 1 if $1 is in std]),
        AC_MSG_RESULT(no)
    )
])

AC_DEFUN([ATF_CHECK_STD_PUTENV], [
    ATF_CHECK_IN_STD([putenv],
                     [#include <cstdio>],
                     [std::putenv("a=b");]
                    )
])

AC_DEFUN([ATF_CHECK_STD_SETENV], [
    ATF_CHECK_IN_STD([setenv],
                     [#include <cstdio>],
                     [std::setenv("a", "b");]
                    )
])

AC_DEFUN([ATF_CHECK_STD_SNPRINTF], [
    ATF_CHECK_IN_STD([snprintf],
                     [#include <cstdio>],
                     [char buf;
                      std::snprintf(&buf, 1, "");]
                    )
])

AC_DEFUN([ATF_CHECK_STD_UNSETENV], [
    ATF_CHECK_IN_STD([unsetenv],
                     [#include <cstdio>],
                     [std::unsetenv("a");]
                    )
])

AC_DEFUN([ATF_CHECK_STD_VSNPRINTF], [
    ATF_CHECK_IN_STD([vsnprintf],
                     [#include <cstdarg>
                      #include <cstdio>],
                     [va_list ap;
                      char* buf = NULL;
                      const char* fmt = NULL;
                      std::vsnprintf(buf, 0, fmt, ap);]
                    )
])
