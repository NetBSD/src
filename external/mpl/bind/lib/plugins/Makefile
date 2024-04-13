#	$NetBSD: Makefile,v 1.6 2024/04/13 17:22:33 christos Exp $

LIBISMODULE=	yes
MAKELINKLIB=	no
.include <bsd.init.mk>

.if !defined(MLIBDIR)
LIB=	filter-aaaa


SHLIB_MAJOR=0
SRCS=	filter-aaaa.c
LIBDIR=	/usr/lib/named

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}
.endif

.include <bsd.lib.mk>
