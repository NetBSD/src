# $NetBSD: Makefile,v 1.11 2001/08/03 14:14:30 drochner Exp $

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

OS_SYSTEM !=	uname -s
OS_RELEASE !=	uname -r
CPPFLAGS+=-DOS_VERSION="\"${OS_SYSTEM} ${OS_RELEASE}\""

.include <bsd.prog.mk>
.include <bsd.files.mk>
