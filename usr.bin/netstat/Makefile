#	$NetBSD: Makefile,v 1.22 2003/05/14 23:36:32 itojun Exp $
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

.include <bsd.prog.mk>
