#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:55 lukem Exp $

.include <bsd.own.mk>

MAN=	GLwCreateMDrawingArea.3 GLwDrawingArea.3 \
	GLwDrawingAreaMakeCurrent.3 GLwDrawingAreaSwapBuffers.3

MLINKS=	GLwDrawingArea.3 GLwMDrawingArea.3

BUILDSYMLINKS=\
	glwcreatemdrawingarea.gl GLwCreateMDrawingArea.man \
	glwdrawingarea.gl GLwDrawingArea.man \
	glwdrawingareamakecurrent.gl GLwDrawingAreaMakeCurrent.man \
	glwdrawingareaswapbuffers.gl GLwDrawingAreaSwapBuffers.man

.PATH:  ${X11SRCDIR.xc}/extras/ogl-sample/main/doc/man/manglw

.include <bsd.x11.mk>
.include <bsd.man.mk>
