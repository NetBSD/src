# $NetBSD: Makefile,v 1.14 2001/08/16 18:03:03 tv Exp $

.include <bsd.own.mk>

SRCS   = dhclient.c clparse.c
PROG   = dhclient
MAN    = dhclient.8 dhclient.conf.5 dhclient.leases.5 dhclient-script.8
CPPFLAGS += -DCLIENT_PATH='"PATH=/usr/bin:/usr/sbin:/bin:/sbin"'

DHCPSRCDIR=	client

# XXX
BINDIR?=	/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
