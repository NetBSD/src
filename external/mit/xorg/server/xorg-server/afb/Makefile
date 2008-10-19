#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:17 haad Exp $

.include "../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		afbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.afb"
