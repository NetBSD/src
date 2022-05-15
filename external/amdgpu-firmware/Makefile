# $NetBSD: Makefile,v 1.2 2022/05/15 00:24:14 mrg Exp $

.include <bsd.own.mk>

.if ${MKAMDGPUFIRMWARE} != "no"
SUBDIR+=	amdgpu
.endif

.if ${MKRADEONFIRMWARE} != "no"
SUBDIR+=	radeon
.endif

.include <bsd.subdir.mk>
