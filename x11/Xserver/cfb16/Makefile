#	$NetBSD: Makefile,v 1.2.4.4 2004/12/07 11:00:32 rtr Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	16

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule16.c
.endif

.include "../Makefile.servermod"
.include "../cfb/Makefile.cfb"
