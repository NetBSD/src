#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

XTODO=	InstallDriverSDKNonExecFile

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/include/GL \
	${X11SRCDIR.xc}/extras/Mesa/include/GL

INCSDIR=${X11INCDIR}/GL

INCS=	gl.h glext.h glu.h glx.h glxext.h glxint.h glxmd.h glxproto.h \
	glxtokens.h osmesa.h

.include <bsd.prog.mk>
.include <bsd.x11.mk>
