#	$NetBSD: Makefile,v 1.4 2003/12/21 03:07:01 rtr Exp $

.include "../Makefile.serverlib"
.include "Makefile.fb"

.if defined(X11FLAGS.LOADABLE)
SUBDIR=	module
.include <bsd.subdir.mk>
.endif
