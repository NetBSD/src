#	$NetBSD: Makefile,v 1.12 2001/02/18 06:15:48 tsutsui Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_dhclient x_dmesg x_ed x_ftp x_gzip x_ifconfig x_mount \
	x_netstat x_ping x_route x_sh x_umount zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
