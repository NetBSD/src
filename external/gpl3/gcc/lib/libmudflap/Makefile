#	$NetBSD: Makefile,v 1.2.2.2 2012/05/23 10:07:28 yamt Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.own.mk>

DIST=           ${NETBSDSRCDIR}/external/gpl3/gcc/dist/libmudflap
CPPFLAGS+=	-I${.CURDIR}
LINTFLAGS+=-g

.PATH: ${DIST}

LIB=mudflap
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c
INCS=mf-runtime.h
INCSDIR=/usr/include/gcc-4.5

.include <bsd.lib.mk>
