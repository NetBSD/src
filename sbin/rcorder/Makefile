#       $NetBSD: Makefile,v 1.2.2.1 1999/12/27 18:30:30 wrstuden Exp $

PROG=   rcorder
SRCS=   ealloc.c hash.c rcorder.c
MAN=	rcorder.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

# XXX hack for make's hash.[ch]
CPPFLAGS+= -DORDER

.include <bsd.prog.mk>
