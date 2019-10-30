# $NetBSD: Makefile,v 1.2 2019/10/30 00:25:00 christos Exp $

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

.if !empty(MACHINE_ARCH:Msh3*)
COPTS.ddns.c +=-Wno-stringop-overflow
.endif

COPTS.omapi.c +=	-Wno-stack-protector

.include <bsd.prog.mk>
