#       $NetBSD: Makefile,v 1.3 2002/06/30 14:17:44 lukem Exp $

PROG=   rcorder
SRCS=   ealloc.c hash.c rcorder.c
MAN=	rcorder.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS?=	3

.include <bsd.prog.mk>
