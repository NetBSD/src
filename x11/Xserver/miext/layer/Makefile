#	$NetBSD: Makefile,v 1.3 2004/10/07 13:47:13 rtr Exp $

.include "../../Makefile.serverlib"
.include "Makefile.layer"

.if defined(X11FLAGS.LOADABLE)
SUBDIR=	module
.include <bsd.subdir.mk>
.endif
