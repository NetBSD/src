#	$NetBSD: Makefile,v 1.4.4.2 2004/11/21 10:01:26 rtr Exp $

.include "../Makefile.serverlib"

SUBDIR=	ext exts

.if ${X11LOADABLE} != "no"
SUBDIR+=	extmod
.include <bsd.subdir.mk>
.endif

