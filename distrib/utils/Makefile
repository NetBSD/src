#	$NetBSD: Makefile,v 1.9 2000/05/12 20:54:16 thorpej Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_dhclient x_ftp x_gzip x_ifconfig x_netstat x_ping \
	x_route x_sh zcat

.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
