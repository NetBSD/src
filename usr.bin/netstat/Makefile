#	$NetBSD: Makefile,v 1.18.4.1 1999/12/27 18:37:06 wrstuden Exp $
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

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>
