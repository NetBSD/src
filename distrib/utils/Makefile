#	$NetBSD: Makefile,v 1.15 2003/01/19 12:35:00 tsutsui Exp $

SUBDIR=	libhack more ssh tls \
	x_dhclient x_dmesg x_ed x_gzip x_ifconfig \
	x_netstat x_ping x_route x_umount zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
