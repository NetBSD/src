#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:04 lukem Exp $

.include <bsd.own.mk>

MAN=	GLwCreateMDrawingArea.3 GLwDrawingArea.3 \
	GLwDrawingAreaMakeCurrent.3 GLwDrawingAreaSwapBuffers.3

MLINKS=	GLwDrawingArea.3 GLwMDrawingArea.3

LNFILES=glwcreatemdrawingarea.gl GLwCreateMDrawingArea.man \
	glwdrawingarea.gl GLwDrawingArea.man \
	glwdrawingareamakecurrent.gl GLwDrawingAreaMakeCurrent.man \
	glwdrawingareaswapbuffers.gl GLwDrawingAreaSwapBuffers.man

.PATH:  ${X11SRCDIR.xc}/extras/ogl-sample/main/doc/man/manglw

.include <bsd.x11.mk>
.include <bsd.man.mk>
