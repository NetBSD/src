# $NetBSD: Makefile,v 1.5 2021/09/26 15:52:40 maya Exp $

.include <bsd.own.mk>

.if ${MKNOUVEAUFIRMWARE} != "no"
SUBDIR+=	gm20x
.endif

.if ${MKTEGRAFIRMWARE} != "no"
SUBDIR+=	tegra
.endif

.include <bsd.subdir.mk>
