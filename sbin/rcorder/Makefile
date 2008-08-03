#       $NetBSD: Makefile,v 1.7 2008/08/03 07:49:46 lukem Exp $

PROG=   rcorder
SRCS=   hash.c rcorder.c
MAN=	rcorder.8
#XXX
.PATH: /usr/src/lib/libutil
SRCS+=	efun.c
.PATH: /usr/src/lib/libc/string
SRCS+=	strndup.c
CPPFLAGS+=-I/usr/src/lib/libc/include

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
