#	$NetBSD: Makefile,v 1.4.2.2 2000/08/15 23:17:36 he Exp $

SUBDIR=	init_s more ssh tls \
	x_dd x_ftp x_ifconfig x_route x_sh zcat
.if exists(sysinst/arch/${MACHINE})
SUBDIR+=sysinst
.endif

.include <bsd.subdir.mk>
