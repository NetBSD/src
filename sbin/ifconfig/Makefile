#	$NetBSD: Makefile,v 1.24 2005/03/20 00:02:58 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

WARNS=	3
PROG=	ifconfig
MAN=	ifconfig.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

DPADD+=${LIBUTIL}
LDADD+=-lutil

SRCS= ifconfig.c 

SRCS+= af_atalk.c
SRCS+= af_iso.c
SRCS+= af_ns.c

SRCS+= agr.c
SRCS+= ieee80211.c
SRCS+= tunnel.c
SRCS+= vlan.c

.include <bsd.prog.mk>
