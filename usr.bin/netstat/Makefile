#	$NetBSD: Makefile,v 1.13 1997/05/06 20:45:37 gwr Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	netstat
SRCS=	atalk.c if.c inet.c iso.c main.c mbuf.c mroute.c ns.c route.c \
	tp_astring.c unix.c
.PATH.c: ${.CURDIR}/../../sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
