#	$NetBSD: Makefile,v 1.1 2008/08/02 21:19:21 mrg Exp $

.include "../Makefile.serverlib"

PIXELSIZE=	32

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule32.c
.endif

.include "../Makefile.servermod"
.include "../cfb/Makefile.cfb"
