#	$NetBSD: Makefile,v 1.4.4.4 2004/12/07 11:00:32 rtr Exp $

.include "../Makefile.serverlib"

SUBDIR=	ext exts

.if ${MKX11LOADABLE} != "no"
SUBDIR+=	extmod
.endif

.include <bsd.subdir.mk>
