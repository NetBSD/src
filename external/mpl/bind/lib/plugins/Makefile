#	$NetBSD: Makefile,v 1.1 2022/03/06 17:50:19 christos Exp $

LIBISMODULE:=	yes
.include <bsd.init.mk>

SHLIB_MAJOR=0
LIB=	filter-aaaa
SRCS=	filter-aaaa.c
LIBDIR=	/usr/lib/named

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}

.if ${MKPIC} != "no"
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
