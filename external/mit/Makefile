#	$NetBSD: Makefile,v 1.6 2020/03/03 00:36:29 christos Exp $

.include <bsd.own.mk>

SUBDIR+=	expat lua libcbor

.if ${MKX11} != "no"
SUBDIR+=	xorg
SUBDIR+=	.WAIT ctwm
.endif

.include <bsd.subdir.mk>
