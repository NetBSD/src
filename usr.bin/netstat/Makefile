#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93
#	$Id: Makefile,v 1.9 1994/08/03 20:34:13 deraadt Exp $

PROG=	netstat
SRCS=	if.c inet.c iso.c main.c mbuf.c mroute.c ns.c route.c \
	unix.c
#SRCS+=	tp_astring.c	# XXXX
.PATH:	${.CURDIR}/../../sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
