#	$NetBSD: mod.mk,v 1.3 2004/12/29 04:16:17 christos Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

LIBDIR=/usr/lib/security
WARNS=3

.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}

LIB_ROOT_DIR= ${.CURDIR}/../../..

LIBUTILDIR!=  cd ${LIB_ROOT_DIR}/libutil; ${PRINTOBJDIR}
LIBCRYPTDIR!=  cd ${LIB_ROOT_DIR}/libcrypt; ${PRINTOBJDIR}
LIBRPCSVCDIR!=  cd ${LIB_ROOT_DIR}/librpcsvc; ${PRINTOBJDIR}

.if (${MKKERBEROS} != "no")
LIBCRYPTODIR!=  cd ${LIB_ROOT_DIR}/libcrypto; ${PRINTOBJDIR}
LIBASN1DIR!=  cd ${LIB_ROOT_DIR}/libasn1; ${PRINTOBJDIR}
LIBROKENDIR!=  cd ${LIB_ROOT_DIR}/libroken; ${PRINTOBJDIR}
LIBCOM_ERRDIR!=  cd ${LIB_ROOT_DIR}/libcom_err; ${PRINTOBJDIR}
LIBKRB5DIR!=  cd ${LIB_ROOT_DIR}/libkrb5; ${PRINTOBJDIR}
.endif


.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
