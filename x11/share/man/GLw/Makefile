#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $

.include <bsd.own.mk>

MAN=	GLwCreateMDrawingArea.3 GLwDrawingArea.3 \
	GLwDrawingAreaMakeCurrent.3 GLwDrawingAreaSwapBuffers.3

MLINKS=	GLwDrawingArea.3 GLwMDrawingArea.3

LNFILES=glwcreatemdrawingarea.gl GLwCreateMDrawingArea.man \
	glwdrawingarea.gl GLwDrawingArea.man \
	glwdrawingareamakecurrent.gl GLwDrawingAreaMakeCurrent.man \
	glwdrawingareaswapbuffers.gl GLwDrawingAreaSwapBuffers.man

.PATH:  ${X11SRCDIR.xc}/extras/ogl-sample/main/doc/man/manglw

.include <bsd.man.mk>
.include <bsd.x11.mk>
