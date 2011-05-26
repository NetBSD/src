#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:27 joerg Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/man

PROG=		diff3
SRCS=		diff3.c version.c

DPADD+=		${LIBDIFFUTILS}
LDADD+=		${LIBDIFFUTILS}

CWARNFLAGS.clang+=	-Wno-unused-value

.include <bsd.prog.mk>
