#	$NetBSD: Makefile,v 1.1 2016/01/13 03:39:28 christos Exp $

.include <bsd.init.mk>

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/man

PROG=		diff3
SRCS=		diff3.c version.c

CWARNFLAGS.clang+=	-Wno-unused-value

.include <bsd.prog.mk>
