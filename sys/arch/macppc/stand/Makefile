#	$NetBSD: Makefile,v 1.8 2024/09/15 03:56:57 tsutsui Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff mkboothfs .WAIT
.endif
SUBDIR+=	bootxx ofwboot
SUBDIR+=	boothfs

.include <bsd.subdir.mk>
