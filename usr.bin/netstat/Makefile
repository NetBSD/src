#	from: @(#)Makefile	5.14 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.5 1994/01/11 19:42:48 brezak Exp $

CFLAGS+=-I${DESTDIR}/sys
PROG=	netstat
SRCS=	inet.c if.c main.c mbuf.c mroute.c route.c unix.c
#
#SRCS+=	host.c
#CFLAGS+= -DIMP

SRCS+=	ns.c
CFLAGS+= -DNS

SRCS+=	iso.c
CFLAGS+= -DISO

BINGRP=	kmem
BINMODE=2555
LDADD=	-lutil
DPADD=	${LIBUTIL}

.include <bsd.prog.mk>
