#	$NetBSD: mod.mk,v 1.4 2004/12/29 15:04:06 lukem Exp $

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

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
