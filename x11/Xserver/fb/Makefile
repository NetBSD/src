#	$NetBSD: Makefile,v 1.5 2003/12/21 13:07:01 mrg Exp $

.include "../Makefile.serverlib"
.include "Makefile.fb"

CPPFLAGS.fbbltone.c=	-Wno-error

.if defined(X11FLAGS.LOADABLE)
SUBDIR=	module
.include <bsd.subdir.mk>
.endif
