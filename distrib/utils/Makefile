#	$NetBSD: Makefile,v 1.6.2.1 1999/12/27 18:28:42 wrstuden Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_ftp x_gzip x_ifconfig x_netstat x_ping x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.include <bsd.subdir.mk>
