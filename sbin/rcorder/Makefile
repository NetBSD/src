#       $NetBSD: Makefile,v 1.2 1999/11/23 05:50:08 mrg Exp $

PROG=   rcorder
SRCS=   ealloc.c hash.c rcorder.c
MAN=	rcorder.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

# XXX hack for make's hash.[ch]
CPPFLAGS+= -DORDER

.include <bsd.prog.mk>
