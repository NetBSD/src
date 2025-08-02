#	$NetBSD: Makefile,v 1.2.40.1 2025/08/02 05:58:27 perseant Exp $

.include <bsd.own.mk>
.include "Makefile.inc"

PROG=	getaddrinfo
SRCS=	getaddrinfo.c support.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

WARNS=	5

.include <bsd.prog.mk>
