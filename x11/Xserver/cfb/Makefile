#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:39 rtr Exp $

NOLINT=	1 # cfb8bit.c

.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
PIXELSIZE=	8
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbcmap.c cfbmodule.c
.endif

.include "../Makefile.serverlib"
.include "Makefile.cfb"
