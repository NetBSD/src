#	$NetBSD: Makefile,v 1.3.4.3 2004/12/07 11:00:33 rtr Exp $

.include "../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		dbemodule.c
.endif

.include "../Makefile.servermod"
.include "Makefile.dbe"
