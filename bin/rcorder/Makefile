#       $NetBSD: Makefile,v 1.1.1.1 1999/11/19 04:30:55 mrg Exp $

PROG=   rcorder
SRCS=   ealloc.c hash.c rcorder.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

# XXX hack for make's hash.[ch]
CPPFLAGS+= -DORDER

# XXX
COPTS+= -g -DDEBUG
LDFLAGS+= -g

# XXX
BINDIR=	/bin

.include <bsd.prog.mk>
