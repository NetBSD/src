#	$NetBSD: Makefile,v 1.3.4.2 2004/11/21 10:01:25 rtr Exp $

.include "../../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
.PATH:          ${X11SRCDIR.xc}/programs/Xserver/GL
SRCS=		glxmodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.glx"
