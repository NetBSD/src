#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:13 mrg Exp $

.include "../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		afbmodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.afb"
