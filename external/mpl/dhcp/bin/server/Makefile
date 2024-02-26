# $NetBSD: Makefile,v 1.7.6.2 2024/02/26 10:31:27 martin Exp $

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

.if !empty(MACHINE_ARCH:Msh3*) || ${MACHINE_ARCH} == "m68000"
COPTS.ddns.c +=		${GCC_NO_STRINGOP_OVERFLOW}
.endif

COPTS.omapi.c +=	-Wno-stack-protector
COPTS.confpars.c+=	${GCC_NO_STRINGOP_TRUNCATION}
COPTS.mdb6.c+=		${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
