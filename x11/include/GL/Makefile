#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:40 lukem Exp $

XTODO=	InstallDriverSDKNonExecFile

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/include/GL \
	${X11SRCDIR.xc}/extras/Mesa/include/GL

INCSDIR=${X11INCDIR}/GL

INCS=	gl.h glext.h glu.h glx.h glxext.h glxint.h glxmd.h glxproto.h \
	glxtokens.h osmesa.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>
