#	$NetBSD: Makefile,v 1.2.4.3 2004/11/21 11:57:47 rtr Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	32

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule32.c
.endif

.include "../Makefile.servermod"
.include "../cfb/Makefile.cfb"
