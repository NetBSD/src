#	$NetBSD: Makefile,v 1.7 2024/04/14 08:29:54 kre Exp $

LIBISMODULE=	yes
MAKELINKLIB=	no
LIB=	filter-aaaa

.include <bsd.init.mk>

SHLIB_MAJOR=0
SRCS=	filter-aaaa.c
LIBDIR=	/usr/lib/named

DIST=${IDIST}/bin/plugins

.PATH: ${DIST}

.include <bsd.lib.mk>
