#	$NetBSD: Makefile,v 1.5 2014/06/01 19:51:01 mrg Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.init.mk>

DIST=		${GCCDIST}/libmudflap
CPPFLAGS+=	-I${.CURDIR}
LINTFLAGS+=-g

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

LIB=mudflap
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c
INCS=mf-runtime.h
INCSDIR=/usr/include/gcc-4.8

.include <bsd.lib.mk>

.PATH: ${DIST}
