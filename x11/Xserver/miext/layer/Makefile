#	$NetBSD: Makefile,v 1.2.4.1 2005/01/13 16:34:42 he Exp $

.include "../../Makefile.serverlib"
.include "Makefile.layer"

.if defined(X11FLAGS.LOADABLE)
SUBDIR=	module
.include <bsd.subdir.mk>
.endif
