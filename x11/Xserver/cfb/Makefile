#	$NetBSD: Makefile,v 1.3.4.2 2004/11/21 10:01:26 rtr Exp $

.include "../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
PIXELSIZE=	8
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbcmap.c cfbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.cfb"
