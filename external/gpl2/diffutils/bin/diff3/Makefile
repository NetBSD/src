#	$NetBSD: Makefile,v 1.1.16.1 2020/04/13 07:56:40 martin Exp $

.include <bsd.init.mk>

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/man

PROG=		diff3
SRCS=		diff3.c version.c

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-error=bool-operation

.include <bsd.prog.mk>
