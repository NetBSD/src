#	$NetBSD: Makefile,v 1.7 1999/10/18 20:20:03 pk Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_ftp x_gzip x_ifconfig x_netstat x_ping x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.include <bsd.subdir.mk>
