#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93
#	$Id: Makefile,v 1.7 1994/05/13 08:08:09 mycroft Exp $

PROG=	netstat
SRCS=	if.c inet.c iso.c main.c mbuf.c mroute.c ns.c route.c \
	unix.c
#SRCS+=	tp_astring.c	# XXXX
CFLAGS+=-I/sys
.PATH:	${.CURDIR}/../../sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
