#	$NetBSD: Makefile,v 1.31.18.1 2008/06/02 13:21:22 mjf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig/Makefile.

.include <bsd.own.mk>

PROG=	ifconfig
MAN=	ifconfig.8

WARNS=	4

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

DPADD+=${LIBUTIL}
DPADD+=${LIBPROP}
LDADD+=-lutil
LDADD+=-lprop

INCS=af_atalk.h af_inet.h af_inetany.h af_iso.h af_link.h agr.h carp.h env.h
INCS+=extern.h ieee80211.h parse.h tunnel.h util.h vlan.h

SRCS= ifconfig.c 

SRCS+= af_atalk.c
SRCS+= af_inet.c
SRCS+= af_inetany.c
.if (${USE_INET6} != "no")
SRCS+= af_inet6.c
INCS+= af_inet6.h
.endif
SRCS+= af_link.c
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
