#	$NetBSD: Makefile,v 1.4 2003/12/21 03:07:01 rtr Exp $

SUBDIR=	ext exts

.include "../Makefile.serverlib"
.include "Makefile.ext"

.if defined(X11FLAGS.LOADABLE)
SUBDIR+=	extmod
.endif

.include <bsd.subdir.mk>
