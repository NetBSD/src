#	$NetBSD: Makefile.inc,v 1.7 2009/09/16 14:59:45 tsutsui Exp $

# shared stuff with src/distrib/utils/x_ifconfig for install media.
# stuff not required by install media should be into Makefile.

PROG=	ifconfig

DPADD+=${LIBUTIL}
DPADD+=${LIBPROP}
LDADD+=-lutil
LDADD+=-lprop

INCS+=af_inetany.h env.h extern.h media.h parse.h util.h
SRCS+= af_inet.c
SRCS+= af_inetany.c
SRCS+= agr.c
SRCS+= env.c
SRCS+= ieee80211.c
SRCS+= ifconfig.c
SRCS+= media.c
SRCS+= parse.c
SRCS+= tunnel.c
SRCS+= util.c
SRCS+= vlan.c
