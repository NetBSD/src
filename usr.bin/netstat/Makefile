#	$NetBSD: Makefile,v 1.21 2002/09/18 14:00:40 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

PROG=	netstat
SRCS=	atalk.c if.c inet.c inet6.c ipsec.c iso.c main.c mbuf.c mroute.c \
	mroute6.c ns.c route.c tp_astring.c unix.c
.PATH:	${NETBSDSRCDIR}/sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}
CPPFLAGS+= -DINET6 -DIPSEC

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>
