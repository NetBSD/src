#	$NetBSD: Makefile,v 1.1 2008/08/03 22:11:10 mrg Exp $

.include "../../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
.PATH:          ${X11SRCDIR.xorg-server}/GL
SRCS=		glxmodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.glx"
