#	$NetBSD: Makefile,v 1.4.4.3 2004/12/07 11:00:32 rtr Exp $

.include "../../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
.PATH:		${X11SRCDIR.xc}/programs/Xserver/GL/mesa/GLcore
SRCS=		GLcoremodule.c
.endif

.include "../../Makefile.servermod"
.include "Makefile.GLcore"
