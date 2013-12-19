#	$NetBSD: Makefile,v 1.5.40.1 2013/12/19 01:15:09 matt Exp $

CONFIGFILES=	NetBSD.dist special
.if exists(${.CURDIR}/NetBSD.dist.${MACHINE_ARCH})
CONFIGFILES+=	NetBSD.dist.${MACHINE_ARCH}
.endif
FILESDIR=	/etc/mtree

.include <bsd.prog.mk>
