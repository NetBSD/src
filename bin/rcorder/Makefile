#       $NetBSD: Makefile,v 1.2 1999/11/19 09:57:39 lukem Exp $

PROG=   rcorder
SRCS=   ealloc.c hash.c rcorder.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

# XXX hack for make's hash.[ch]
CPPFLAGS+= -DORDER

.include <bsd.prog.mk>
