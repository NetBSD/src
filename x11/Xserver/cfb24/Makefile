#	$NetBSD: Makefile,v 1.2 2003/12/21 03:07:01 rtr Exp $

PIXELSIZE=	24

.include "../Makefile.serverlib"
.include "../cfb/Makefile.cfb"

.if defined(X11FLAGS.LOADABLE)
SUBDIR= module
.include <bsd.subdir.mk>
.endif
