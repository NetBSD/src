#	$NetBSD: Makefile,v 1.15.2.1 2004/06/17 08:03:04 tron Exp $

SUBDIR=	libhack more ssh tls \
	x_dhclient x_dmesg x_ed x_gzip x_ifconfig \
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
