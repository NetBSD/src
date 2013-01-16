#	$NetBSD: Makefile.inc,v 1.8.6.1 2013/01/16 05:32:33 yamt Exp $

# shared stuff with src/distrib/utils/x_ifconfig for install media.
# stuff not required by install media should be into Makefile.

DPADD+=${LIBUTIL}
DPADD+=${LIBPROP}
LDADD+=-lutil
LDADD+=-lprop

INCS+=af_inetany.h env.h extern.h media.h parse.h util.h
SRCS+= af_inet.c
SRCS+= af_inetany.c
SRCS+= agr.c
SRCS+= env.c
SRCS+= ether.c
SRCS+= ieee80211.c
SRCS+= ifconfig.c
SRCS+= media.c
SRCS+= parse.c
SRCS+= tunnel.c
SRCS+= util.c
SRCS+= vlan.c
