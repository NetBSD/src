#	$NetBSD: Makefile,v 1.17 2005/09/10 13:24:38 dsl Exp $

SUBDIR=	libhack more ssh tls \
	x_dhclient x_ed x_gzip x_ifconfig \
	x_netstat x_ping x_route x_umount zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.if ${MACHINE} == "acorn32"
SUBDIR+=sparkcrc
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
