#	$NetBSD: Makefile,v 1.4 2014/03/01 10:00:42 mrg Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.own.mk>

DIST=           ${NETBSDSRCDIR}/external/gpl3/gcc/dist/libmudflap
CPPFLAGS+=	-I${.CURDIR}
LINTFLAGS+=-g

.PATH: ${DIST}

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

LIB=mudflap
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c
INCS=mf-runtime.h
INCSDIR=/usr/include/gcc-4.8

.include <bsd.lib.mk>
