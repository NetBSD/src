#	$NetBSD: Makefile,v 1.14 2002/10/17 06:38:23 lukem Exp $

SUBDIR=	libhack more ssh tls \
	x_dhclient x_dmesg x_ed x_gzip x_ifconfig x_mount \
	x_netstat x_ping x_route x_umount zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
