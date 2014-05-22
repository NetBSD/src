# $NetBSD: Makefile,v 1.1.10.2 2014/05/22 15:44:31 yamt Exp $

.include <bsd.own.mk>

SRCS   = dhcpd.c dhcp.c dhcpv6.c bootp.c confpars.c db.c class.c failover.c \
         omapi.c mdb.c mdb6.c stables.c salloc.c ddns.c dhcpleasequery.c
PROG   = dhcpd
MAN    = dhcpd.8 dhcpd.conf.5 dhcpd.leases.5

DHCPSRCDIR=	server

.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/dhcp

FILES=	dhcpd.conf
.endif

.include <bsd.prog.mk>
