# $NetBSD: Makefile,v 1.6 2021/12/13 23:59:35 maya Exp $

.include <bsd.own.mk>

.if ${MKNOUVEAUFIRMWARE} != "no"
SUBDIR+=	gm20x
SUBDIR+=	nouveau
.endif

.if ${MKTEGRAFIRMWARE} != "no"
SUBDIR+=	tegra
.endif

.include <bsd.subdir.mk>
