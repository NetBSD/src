#	$NetBSD: Makefile,v 1.2 2019/11/11 22:45:26 joerg Exp $

.include <bsd.init.mk>

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/man

PROG=		diff3
SRCS=		diff3.c version.c

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-error=bool-operation

.include <bsd.prog.mk>
