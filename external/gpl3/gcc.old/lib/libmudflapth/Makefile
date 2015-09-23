#	$NetBSD: Makefile,v 1.1.1.3 2015/09/23 03:03:38 mrg Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.init.mk>

DIST=		${GCCDIST}/libmudflap
CPPFLAGS+=	-I${.CURDIR}/../libmudflap -DLIBMUDFLAPTH

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

LINTFLAGS+=-g
LIB=mudflapth
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c mf-hooks3.c

.include <bsd.lib.mk>

.PATH: ${DIST}
