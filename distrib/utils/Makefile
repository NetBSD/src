#	$NetBSD: Makefile,v 1.13 2002/10/17 06:02:34 lukem Exp $

SUBDIR=	libhack more ssh tls \
	x_dhclient x_dmesg x_ed x_ftp x_gzip x_ifconfig x_mount \
	x_netstat x_ping x_route x_umount zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
