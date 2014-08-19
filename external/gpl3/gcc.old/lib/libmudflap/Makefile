#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:15 tls Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.init.mk>

DIST=           ${GCCDIST}/libmudflap
CPPFLAGS+=	-I${.CURDIR}
LINTFLAGS+=-g

LIB=mudflap
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c
INCS=mf-runtime.h
INCSDIR=/usr/include/gcc-4.5

.include <bsd.lib.mk>

.PATH: ${DIST}
