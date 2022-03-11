#	$NetBSD: Makefile,v 1.4 2022/03/11 16:24:22 christos Exp $

LIBISMODULE:=	yes
LIB=	filter-aaaa

.include <bsd.init.mk>

SHLIB_MAJOR=0
SRCS=	filter-aaaa.c
LIBDIR=	/usr/lib/named

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}

.if !defined(MLIBDIR) && ${MKPIC} != "no"
.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}
.else
libinstall::
.endif

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}: ${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
