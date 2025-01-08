# SYNOPSIS
#
#   AX_CHECK_OPENSSL_DEFINE([DEFINE], [VAR][, action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Check if OpenSSL has a define set in it's features provided, i.e. OPENSSL_NO_MD4.
#   If so, the var argument ac_cv_openssl_[VAR] is set to yes, and action-is-found is
#   run, else action-if-not-found is executed.
#
#   This module require AX_CHECK_OPENSSL
#
# LICENSE
#
#   Copyright (c) 2021 Eivind Naess <eivnaes@yahoo.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([AX_CHECK_OPENSSL_DEFINE], [
    AC_REQUIRE([AX_CHECK_OPENSSL])
    AC_MSG_CHECKING([for $2 support in openssl])
    save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$OPENSSL_INCLUDES $CPPFLAGS"
    AC_PREPROC_IFELSE([
        AC_LANG_PROGRAM(
            [[@%:@include <openssl/opensslconf.h>]],
            [[#ifdef $1
              #error "No support for $1"
              #endif]])],
        AC_MSG_RESULT([yes])
        [ac_cv_openssl_$2=yes]
        $3,
        AC_MSG_RESULT([no])
        [ac_cv_openssl_$2=no]
        $4
    )
    CPPFLAGS="$save_CPPFLAGS"
])

