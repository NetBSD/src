#	from: @(#)Makefile	5.14 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.6 1994/01/28 00:48:33 cgd Exp $

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
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
