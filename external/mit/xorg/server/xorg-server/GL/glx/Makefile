#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:43 wrstuden Exp $

.include "../../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
.PATH:          ${X11SRCDIR.xorg-server}/GL
SRCS=		glxmodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.glx"
