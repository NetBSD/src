#	$NetBSD: Makefile,v 1.3 2022/03/11 13:39:01 christos Exp $

LIBISMODULE:=	yes
LIB=	filter-aaaa

.include <bsd.init.mk>

SHLIB_MAJOR=0
SRCS=	filter-aaaa.c
LIBDIR=	/usr/lib/named

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}

.if ${MLIBDIR:Uno} != no && ${MKPIC} != "no"
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
