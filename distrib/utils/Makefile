#	$NetBSD: Makefile,v 1.4.2.1 1999/04/17 11:05:44 simonb Exp $

SUBDIR=	libhack init_s more ssh tls \
	x_dd x_ftp x_ifconfig x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.include <bsd.subdir.mk>
