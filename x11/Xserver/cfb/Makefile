#	$NetBSD: Makefile,v 1.3.4.3 2004/11/21 11:57:47 rtr Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	8

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbcmap.c cfbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.cfb"
