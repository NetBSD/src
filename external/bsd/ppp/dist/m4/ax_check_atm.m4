# SYNOPSIS
#
#   AX_CHECK_ATM([action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Look for libatm in a number of default locations, or in a provided location
#   (via --with-atm=). Sets
#       ATM_CFLAGS
#       ATM_LDFLAGS
#       ATM_LIBS
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

AC_DEFUN([AX_CHECK_ATM], [
    AC_ARG_WITH([atm],
        [AS_HELP_STRING([--with-atm=DIR],
            [With libatm support, see http://linux-atm.sourceforge.net])],
        [
            case "$withval" in
            "" | y | ye | yes)
                atmdirs="/usr/local /usr/lib /usr"  
              ;;
            n | no)
                with_atm="no"
              ;;
            *)
                atmdirs="$withval"
              ;;
            esac
        ])
    
    if [ test "x${with_atm}" != "xno" ] ; then
        ATM_LIBS="-latm"
        for atmdir in $atmdirs; do
            AC_MSG_CHECKING([for atm.h in $atmdir])
            if test -f "$atmdir/include/atm.h"; then
                ATM_CFLAGS="-I$atmdir/include"
                ATM_LDFLAGS="-L$atmdir/lib"
                AC_MSG_RESULT([yes])
                break
            else
                AC_MSG_RESULT([no])
            fi
        done

        # try the preprocessor and linker with our new flags,
        # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

        AC_MSG_CHECKING([if compiling and linking against libatm works])

        save_LIBS="$LIBS"
        save_LDFLAGS="$LDFLAGS"
        save_CPPFLAGS="$CPPFLAGS"
        LDFLAGS="$LDFLAGS $ATM_LDFLAGS"
        LIBS="$ATM_LIBS $LIBS"
        CPPFLAGS="$ATM_CFLAGS $CPPFLAGS"
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM(
                [#include <atm.h>
                 #include <stddef.h>], 
                [text2atm(NULL,NULL,0,0);])],
            [
                AC_MSG_RESULT([yes])
                with_atm=yes
                $1
            ], [
                AC_MSG_RESULT([no])
                with_atm="no"
                $2
            ])
        CPPFLAGS="$save_CPPFLAGS"
        LDFLAGS="$save_LDFLAGS"
        LIBS="$save_LIBS"

        AC_SUBST([ATM_CFLAGS])
        AC_SUBST([ATM_LIBS])
        AC_SUBST([ATM_LDFLAGS])
    fi
    AM_CONDITIONAL(WITH_LIBATM, test "x${with_atm}" != "xno")
])

