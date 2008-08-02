#	$NetBSD: Makefile,v 1.1 2008/08/02 04:32:00 mrg Exp $

.include "../../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		laymodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.rootless"
