#	$NetBSD: Makefile,v 1.2.4.2 2004/11/21 10:01:26 rtr Exp $

.include "../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
PIXELSIZE=	16
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule16.c
.endif

.include "../Makefile.servermod"
.include "../cfb/Makefile.cfb"
