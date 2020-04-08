#	$NetBSD: Makefile,v 1.5.16.1 2020/04/08 14:07:01 martin Exp $

.include <bsd.own.mk>

SUBDIR+=	expat lua libcbor

.if ${MKX11} != "no"
SUBDIR+=	xorg
SUBDIR+=	.WAIT ctwm
.endif

.include <bsd.subdir.mk>
