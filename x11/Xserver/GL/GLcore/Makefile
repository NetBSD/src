#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:38 rtr Exp $

.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
.PATH:		${X11SRCDIR.xc}/programs/Xserver/GL/mesa/GLcore
SRCS=		GLcoremodule.c
.endif

.include "../../Makefile.serverlib"
.include "Makefile.GLcore"
