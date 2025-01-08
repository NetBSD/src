# SYNOPSIS
#
#   AX_CHECK_SRP([action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Look for libsrp in a number of default locations, or in a provided location
#   (via --with-srp=). Sets
#       SRP_CFLAGS
#       SRP_LDFLAGS
#       SRP_LIBS
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

AC_DEFUN([AX_CHECK_SRP], [
    AC_ARG_WITH([srp],
        [AS_HELP_STRING([--with-srp=DIR],
            [With libsrp support, see http://srp.stanford.edu])],
        [
            case "$withval" in
            "" | y | ye | yes)
                srpdirs="/usr/local /usr/lib /usr"  
              ;;
            n | no)
                with_srp="no"
              ;;
            *)
                srpdirs="$withval"
              ;;
            esac
        ])
    
    if [ test "x${with_srp}" != "xno" ] ; then
        SRP_LIBS="-lsrp"
        for srpdir in $srpdirs; do
            AC_MSG_CHECKING([for srp.h in $srpdir])
            if test -f "$srpdir/include/srp.h"; then
                SRP_CFLAGS="-I$srpdir/include"
                SRP_LDFLAGS="-L$srpdir/lib"
                AC_MSG_RESULT([yes])
                break
            else
                AC_MSG_RESULT([no])
            fi
        done

        # try the preprocessor and linker with our new flags,
        # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

        AC_MSG_CHECKING([if compiling and linking against libsrp works])

        save_LIBS="$LIBS"
        save_LDFLAGS="$LDFLAGS"
        save_CPPFLAGS="$CPPFLAGS"
        LDFLAGS="$SRP_LDFLAGS $OPENSSL_LDFLAGS $LDFLAGS"
        LIBS="$SRP_LIBS $OPENSSL_LIBS $LIBS"
        CPPFLAGS="$SRP_CFLAGS $OPENSSL_INCLUDES $CPPFLAGS"
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM(
                [#include <srp.h>
                 #include <stddef.h>], 
                [SRP_use_engine(NULL);])],
            [
                AC_MSG_RESULT([yes])
                with_srp=yes
                $1
            ], [
                AC_MSG_RESULT([no])
                with_srp="no"
                $2
            ])
        CPPFLAGS="$save_CPPFLAGS"
        LDFLAGS="$save_LDFLAGS"
        LIBS="$save_LIBS"

        AC_SUBST([SRP_CFLAGS])
        AC_SUBST([SRP_LIBS])
        AC_SUBST([SRP_LDFLAGS])
    fi

    AM_CONDITIONAL(WITH_SRP, test "x${with_srp}" != "xno")
])

