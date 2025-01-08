# SYNOPSIS
#
#   AX_CHECK_PAM([action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Look for libpam in a number of default locations, or in a provided location
#   (via --with-pam=). Sets
#       PAM_CFLAGS
#       PAM_LDFLAGS
#       PAM_LIBS
#
#   and calls ACTION-IF-FOUND or ACTION-IF-NOT-FOUND appropriately
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

AC_DEFUN([AX_CHECK_PAM], [
    AC_ARG_WITH([pam],
        [AS_HELP_STRING([--with-pam=yes|no|DIR],
            [With libpam support, see ftp.redhat.com:/pub/pam])])

    AS_CASE(["$with_pam"],
        [ye|y], [with_pam=yes],
        [n], [with_pam=no])
    
    AS_IF([test "x$with_pam" != "xno"], [
        AS_CASE(["$with_pam"],
            [""|yes], [PKG_CHECK_MODULES([PAM], [pam], [pamdirs=],
                        [pamdirs="/usr/local /usr/lib /usr"])],
            [pamdirs="$with_pam"])

        AS_IF([test -n "$pamdirs"], [
            PAM_LIBS="-lpam"
            for pamdir in $pamdirs; do
                AC_MSG_CHECKING([for pam_appl.h in $pamdir])
                if test -f "$pamdir/include/security/pam_appl.h"; then
                    PAM_CFLAGS="-I$pamdir/include"
                    PAM_LDFLAGS="-L$pamdir/lib"
                    AC_MSG_RESULT([yes])
                    break
                else
                    AC_MSG_RESULT([no])
                fi
            done
        ])

        # try the preprocessor and linker with our new flags,
        # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

        AC_MSG_CHECKING([if compiling and linking against libpam works])

        save_LIBS="$LIBS"
        save_LDFLAGS="$LDFLAGS"
        save_CPPFLAGS="$CPPFLAGS"
        LDFLAGS="$LDFLAGS $PAM_LDFLAGS"
        LIBS="$PAM_LIBS $LIBS"
        CPPFLAGS="$PAM_CFLAGS $CPPFLAGS"
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM(
                [#include <security/pam_appl.h>
                 #include <stddef.h>], 
                [pam_authenticate(NULL, 0);])],
            [
                AC_MSG_RESULT([yes])
                with_pam=yes
                $1
            ], [
                AC_MSG_RESULT([no])
                with_pam="no"
                $2
            ])
        CPPFLAGS="$save_CPPFLAGS"
        LDFLAGS="$save_LDFLAGS"
        LIBS="$save_LIBS"

        AC_SUBST([PAM_CFLAGS])
        AC_SUBST([PAM_LIBS])
        AC_SUBST([PAM_LDFLAGS])
    ])
    AM_CONDITIONAL(WITH_LIBPAM, test "x${with_pam}" != "xno")
])

