# $NetBSD: Makefile,v 1.4 2021/09/25 08:54:30 maya Exp $

.include <bsd.own.mk>

.if ${MKGPUFIRMWARE} != "no"
SUBDIR+=	gm20x
.endif

.if ${MKTEGRAFIRMWARE} != "no"
SUBDIR+=	tegra
.endif

.include <bsd.subdir.mk>
