#	$NetBSD: Makefile,v 1.5 2024/04/05 01:16:01 christos Exp $

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
