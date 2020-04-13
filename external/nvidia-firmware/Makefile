# $NetBSD: Makefile,v 1.1.4.1 2020/04/13 08:03:04 martin Exp $

.include <bsd.own.mk>

.if ${MKNOUVEAUFIRMWARE} != "no"
SUBDIR+=	gm20x
.endif

.if ${MKTEGRAFIRMWARE} != "no"
SUBDIR+=	tegra
.endif

.include <bsd.subdir.mk>
