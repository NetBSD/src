# $NetBSD: Makefile,v 1.13.34.1 2008/09/18 04:30:03 wrstuden Exp $

.include <bsd.own.mk>

SRCS   = dhcpd.c dhcp.c bootp.c confpars.c db.c class.c failover.c \
         omapi.c mdb.c stables.c salloc.c ddns.c
PROG   = dhcpd
MAN    = dhcpd.8 dhcpd.conf.5 dhcpd.leases.5

DHCPSRCDIR=	server

.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/dhcp

FILES=	dhcpd.conf
.endif

.if defined(HAVE_PCC)
COPTS += -Wno-unreachable-code
.endif

.include <bsd.prog.mk>
