#	$NetBSD: Makefile,v 1.1.1.1 2003/01/26 00:40:40 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/man

PROG=		cmp
SRCS=		cmp.c version.c

DPADD+=		${LIBDIFFUTILS}
LDADD+=		${LIBDIFFUTILS}

.include <bsd.prog.mk>
