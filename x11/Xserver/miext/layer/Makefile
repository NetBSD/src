#	$NetBSD: Makefile,v 1.3.2.2 2004/11/21 10:01:28 rtr Exp $

.include "../../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		laymodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.layer"
