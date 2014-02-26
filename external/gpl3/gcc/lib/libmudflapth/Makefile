#	$NetBSD: Makefile,v 1.3 2014/02/26 09:54:33 mrg Exp $

NOLINT=yes			# core dumps
UNSUPPORTED_COMPILER.clang=     # defined

.include <bsd.init.mk>

DIST=           ${GCCDIST}/libmudflap
CPPFLAGS+=	-I${.CURDIR}/../libmudflap -DLIBMUDFLAPTH

LINTFLAGS+=-g
LIB=mudflapth
SRCS=mf-runtime.c mf-heuristics.c mf-hooks1.c mf-hooks2.c mf-hooks3.c

.include <bsd.lib.mk>

.PATH: ${DIST}
