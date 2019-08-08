#	$NetBSD: Makefile,v 1.1 2019/08/08 14:00:28 kamil Exp $

.include <bsd.own.mk>

SUBDIR+=	include share

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	lib
.endif

.include <bsd.subdir.mk>
