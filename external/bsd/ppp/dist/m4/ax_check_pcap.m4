# SYNOPSIS
#
#   AX_CHECK_PCAP([action-if-found[, action-if-not-found]]
#
# DESCRIPTION
#
#   Look for libpcap in a number of default locations, or in a provided location
#   (via --with-pcap=). Sets
#       PCAP_CFLAGS
#       PCAP_LDFLAGS
#       PCAP_LIBS
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

AC_DEFUN([AX_CHECK_PCAP], [
    AC_ARG_WITH([pcap],
        [AS_HELP_STRING([--with-pcap=DIR],
            [With libpcap support, see https://www.tcpdump.org])],
        [
            case "$withval" in
            "" | y | ye | yes)
                pcapdirs="/usr/local /usr/lib /usr"  
              ;;
            n | no)
                with_pcap="no"
              ;;
            *)
                pcapdirs="$withval"
              ;;
            esac
        ])
    
    if [ test "x${with_pcap}" != "xno" ] ; then
        PCAP_LIBS="-lpcap"
        for pcapdir in $pcapdirs; do
            AC_MSG_CHECKING([for pcap.h in $pcapdir])
            if test -f "$pcapdir/include/pcap.h"; then
                PCAP_CFLAGS="-I$pcapdir/include"
                PCAP_LDFLAGS="-L$pcapdir/lib"
                AC_MSG_RESULT([yes])
                break
            else
                AC_MSG_RESULT([no])
            fi
        done

        # try the preprocessor and linker with our new flags,
        # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

        AC_MSG_CHECKING([if compiling and linking against libpcap works])

        save_LIBS="$LIBS"
        save_LDFLAGS="$LDFLAGS"
        save_CPPFLAGS="$CPPFLAGS"
        LDFLAGS="$PCAP_LDFLAGS $LDFLAGS"
        LIBS="$PCAP_LIBS $LIBS"
        CPPFLAGS="$PCAP_CFLAGS $CPPFLAGS"
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM(
                [@%:@include <pcap.h>],
                [pcap_create(0,0);])],
            [
                AC_MSG_RESULT([yes])
                with_pcap=yes
                $1
            ], [
                AC_MSG_RESULT([no])
                with_pcap="no"
                $2
            ])
        CPPFLAGS="$save_CPPFLAGS"
        LDFLAGS="$save_LDFLAGS"
        LIBS="$save_LIBS"

        AC_SUBST([PCAP_CFLAGS])
        AC_SUBST([PCAP_LIBS])
        AC_SUBST([PCAP_LDFLAGS])
    fi

    AM_CONDITIONAL(WITH_PCAP, test "x${with_pcap}" != "xno")
])

