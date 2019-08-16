#	$NetBSD: Makefile,v 1.1.2.2 2019/08/16 18:44:06 martin Exp $

.include <bsd.own.mk>

SUBDIR+=	include share

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	lib
.endif

.include <bsd.subdir.mk>
