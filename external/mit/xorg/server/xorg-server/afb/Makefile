#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:44 wrstuden Exp $

.include "../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		afbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.afb"
