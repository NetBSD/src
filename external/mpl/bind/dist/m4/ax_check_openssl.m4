# ===========================================================================
#     https://www.gnu.org/software/autoconf-archive/ax_check_openssl.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_OPENSSL([action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Look for OpenSSL in a number of default spots, or in a user-selected
#   spot (via --with-openssl).  Sets
#
#     OPENSSL_CFLAGS to the include directives required
#     OPENSSL_LIBS to the -l directives required
#
#   and calls ACTION-IF-FOUND or ACTION-IF-NOT-FOUND appropriately
#
#   This macro sets OPENSSL_CFLAGS such that source files should use the
#   openssl/ directory in include directives:
#
#     #include <openssl/hmac.h>
#
# LICENSE
#
#   Copyright (c) 2009,2010 Zmanda Inc. <http://www.zmanda.com/>
#   Copyright (c) 2009,2010 Dustin J. Mitchell <dustin@zmanda.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 10

AU_ALIAS([CHECK_SSL], [AX_CHECK_OPENSSL])
AC_DEFUN([AX_CHECK_OPENSSL], [
    found=false
    default_ssldirs="/usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /opt/local /usr/local/opt/openssl /usr/local/opt/libressl /usr"
    AC_ARG_WITH([openssl],
        [AS_HELP_STRING([--with-openssl=DIR],
            [root of the OpenSSL directory])],
        [
	    AS_CASE([$with_openssl],
	        [""|y|ye|yes],[ssldirs="$default_ssldirs"],
	    	[n|no],[AC_MSG_ERROR([Invalid --with-openssl value])],
            	[*],[ssldirs="$withval"],
	    	[ssldirs="$default_ssldirs"]
	    )
        ], [
            # if pkg-config is installed and openssl has installed a .pc file,
            # then use that information and don't search ssldirs
	    PKG_CHECK_MODULES([OPENSSL], [crypto],
 	        [found=true],
	    	[ssldirs="$default_ssldirs"])
	
        ]
    )


    # note that we #include <openssl/foo.h>, so the OpenSSL headers have to be in
    # an 'openssl' subdirectory

    AS_IF([! $found],[
        OPENSSL_CFLAGS=
        for ssldir in $ssldirs; do
            AC_MSG_CHECKING([for openssl/ssl.h in $ssldir])
	    AS_IF([test -f "$ssldir/include/openssl/ssl.h"],
	        [
		    OPENSSL_CFLAGS="-I$ssldir/include"
                    OPENSSL_LIBS="-L$ssldir/lib -lcrypto"
                    found=true
                    AC_MSG_RESULT([yes])
                    break
		],
		[
		    AC_MSG_RESULT([no])
		])
        done

        # if the file wasn't found, well, go ahead and try the link anyway -- maybe
        # it will just work!
    ])

    # try the preprocessor and linker with our new flags,
    # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

    AC_MSG_CHECKING([whether compiling and linking against OpenSSL works])
    # AC_MSG_NOTICE([Trying link with OPENSSL_LIBS=$OPENSSL_LIBS; OPENSSL_CFLAGS=$OPENSSL_CFLAGS])

    save_LIBS="$LIBS"
    save_CPPFLAGS="$CPPFLAGS"
    LIBS="$OPENSSL_LIBS $LIBS"
    CPPFLAGS="$OPENSSL_CFLAGS $CPPFLAGS"
    AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [
                #include <openssl/crypto.h>
            ],
            [
	        OPENSSL_free(NULL);
	    ])],
        [
            AC_MSG_RESULT([yes])
            $1
        ], [
            AC_MSG_RESULT([no])
            $2
        ])
    CPPFLAGS="$save_CPPFLAGS"
    LIBS="$save_LIBS"

    AC_SUBST([OPENSSL_CFLAGS])
    AC_SUBST([OPENSSL_LIBS])
])
