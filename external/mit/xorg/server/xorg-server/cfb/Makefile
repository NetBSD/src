#	$NetBSD: Makefile,v 1.1 2008/08/02 21:19:21 mrg Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	8

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.cfb"
