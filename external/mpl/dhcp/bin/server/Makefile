# $NetBSD: Makefile,v 1.1.4.1 2020/04/13 08:03:03 martin Exp $

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

COPTS.mdb6.c +=		${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-error=format-overflow :}
COPTS.omapi.c +=	-Wno-stack-protector

.include <bsd.prog.mk>
