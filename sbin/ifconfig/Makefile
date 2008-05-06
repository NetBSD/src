#	$NetBSD: Makefile,v 1.34 2008/05/06 16:07:00 dyoung Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig/Makefile.

.include <bsd.own.mk>

PROG=	ifconfig
MAN=	ifconfig.8

WARNS=	4

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/ -Wall -Wno-unused-parameter -pedantic -std=c99

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

DPADD+=${LIBUTIL}
DPADD+=${LIBPROP}
LDADD+=-lutil
LDADD+=-lprop

SRCS= ifconfig.c 

SRCS+= af_atalk.c
SRCS+= af_inet.c
SRCS+= af_inetany.c
.if (${USE_INET6} != "no")
SRCS+= af_inet6.c
.endif
SRCS+= af_iso.c
SRCS+= carp.c

SRCS+= agr.c
SRCS+= ieee80211.c
SRCS+= tunnel.c
SRCS+= vlan.c
SRCS+= parse.c
SRCS+= env.c
SRCS+= util.c

.include <bsd.prog.mk>
