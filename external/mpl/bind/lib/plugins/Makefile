#	$NetBSD: Makefile,v 1.8 2024/04/16 19:15:37 christos Exp $

LIBISMODULE=	yes
MAKELINKLIB=	no
LIB=	filter-aaaa

.include <bsd.init.mk>

SHLIB_MAJOR=0
SRCS=	filter-aaaa.c
.if defined(MLIBDIR)
LIBDIR=	/usr/lib/${MLIBDIR}/named
.else
LIBDIR=	/usr/lib/named
.endif

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}

.include <bsd.lib.mk>
