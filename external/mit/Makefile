#	$NetBSD: Makefile,v 1.7 2020/05/24 19:28:57 christos Exp $

.include <bsd.own.mk>

SUBDIR+=	expat lua libcbor libuv

.if ${MKX11} != "no"
SUBDIR+=	xorg
SUBDIR+=	.WAIT ctwm
.endif

.include <bsd.subdir.mk>
