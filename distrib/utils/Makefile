#	$NetBSD: Makefile,v 1.8 2000/01/04 08:33:51 itojun Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_ftp x_gzip x_ifconfig x_netstat x_ping x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

# IPv6
SUBDIR+=x_ping6

.include <bsd.subdir.mk>
