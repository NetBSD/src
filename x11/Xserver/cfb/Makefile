#	$NetBSD: Makefile,v 1.3 2004/03/04 01:06:57 rtr Exp $

PIXELSIZE=	8

.include "../Makefile.serverlib"

SRCS=	cfbcmap.c

.include "Makefile.cfb"

.if defined(X11FLAGS.LOADABLE)
SUBDIR= module
.include <bsd.subdir.mk>
.endif
