#	$NetBSD: Makefile,v 1.17 2005/01/20 16:16:10 xtraeme Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

WARNS=	3
PROG=	ifconfig
MAN=	ifconfig.8

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

DPADD+=${LIBUTIL}
LDADD+=-lutil

SRCS= ifconfig.c 

.include <bsd.prog.mk>
