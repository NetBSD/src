# $NetBSD: Makefile,v 1.24.18.1 2008/06/02 13:21:19 mjf Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

.include <bsd.own.mk>

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

DPADD+=${LIBUTIL}
LDADD+=-lutil

DPADD+=${LIBPROP}
LDADD+=-lprop

SRCS= ifconfig.c 

SRCS+= af_atalk.c
SRCS+= af_inet.c
SRCS+= af_inetany.c
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
SRCS+= af_inet6.c
.endif
SRCS+= af_link.c
SRCS+= af_iso.c

SRCS+= agr.c
SRCS+= ieee80211.c
SRCS+= env.c
SRCS+= parse.c
SRCS+= util.c
SRCS+= tunnel.c
SRCS+= vlan.c

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
