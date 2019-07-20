# $NetBSD: Makefile,v 1.3 2019/07/20 20:49:06 mrg Exp $

.include <bsd.own.mk>

.if ${MKNOUVEAUFIRMWARE} != "no"
SUBDIR+=	gm20x
.endif

.if ${MKTEGRAFIRMWARE} != "no"
SUBDIR+=	tegra
.endif

.include <bsd.subdir.mk>
