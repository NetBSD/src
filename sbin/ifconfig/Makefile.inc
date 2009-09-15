#	$NetBSD: Makefile.inc,v 1.6 2009/09/15 09:22:07 he Exp $

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
