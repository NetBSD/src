#	$NetBSD: Makefile,v 1.2.2.3 2014/05/22 16:53:27 yamt Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.own.mk>

DIST=           ${NETBSDSRCDIR}/external/gpl3/gcc/dist/libmudflap
CPPFLAGS+=	-I${.CURDIR}/../libmudflap -DLIBMUDFLAPTH

.PATH: ${DIST}

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

LINTFLAGS+=-g
LIB=mudflapth
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c mf-hooks3.c

.include <bsd.lib.mk>
