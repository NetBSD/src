#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:38 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	ext exts

.if ${X11LOADABLE} != "no"
SUBDIR+=	extmod
.include <bsd.subdir.mk>
.endif

