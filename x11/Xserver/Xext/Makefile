#	$NetBSD: Makefile,v 1.4.4.3 2004/11/21 11:57:47 rtr Exp $

.include "../Makefile.serverlib"

SUBDIR=	ext exts

.if ${X11LOADABLE} != "no"
SUBDIR+=	extmod
.endif

.include <bsd.subdir.mk>
