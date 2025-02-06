#	$NetBSD: Makefile,v 1.3 2025/02/06 19:35:28 christos Exp $

.include <bsd.own.mk>
.include "Makefile.inc"

PROG=	getaddrinfo
SRCS=	getaddrinfo.c support.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

WARNS=	5

.include <bsd.prog.mk>
