#	$NetBSD: Makefile,v 1.3.4.4 2004/12/07 11:00:32 rtr Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	8

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.cfb"
