#	$NetBSD: Makefile,v 1.3 2015/01/07 20:42:01 joerg Exp $

.include <bsd.own.mk>

SUBDIR+=	expat lua

.if ${MKX11} != "no" && ${X11FLAVOUR} == "Xorg"
SUBDIR+=	xorg
.endif

.include <bsd.subdir.mk>
