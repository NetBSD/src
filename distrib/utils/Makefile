#	$NetBSD: Makefile,v 1.6 1999/05/23 07:31:20 tron Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_ftp x_gzip x_ifconfig x_netstat x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.include <bsd.subdir.mk>
