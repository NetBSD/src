#	$NetBSD: Makefile,v 1.18 1999/07/12 18:17:56 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	netstat
SRCS=	atalk.c if.c inet.c inet6.c iso.c main.c mbuf.c mroute.c mroute6.c \
	ns.c route.c tp_astring.c unix.c
.PATH:	${.CURDIR}/../../sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}
CPPFLAGS+= -DINET6 -DIPSEC

.include <bsd.prog.mk>
