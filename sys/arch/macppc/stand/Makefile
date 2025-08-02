#	$NetBSD: Makefile,v 1.7.202.1 2025/08/02 05:55:52 perseant Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff mkboothfs .WAIT
.endif
SUBDIR+=	bootxx ofwboot
SUBDIR+=	boothfs

.include <bsd.subdir.mk>
