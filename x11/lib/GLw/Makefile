#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	GLw
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/extras/ogl-sample/main/gfx/lib/glw

SRCS=	GLwDrawA.c GLwM1DrawA.c GLwM2DrawA.c GLwDAUtil.c

INCS=	GLwDrawA.h GLwDrawAP.h GLwMDrawA.h GLwMDrawAP.h
INCSDIR=${X11INCDIR}/GL

CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/${LIB} \
		-I${X11SRCDIR.xc}/extras/ogl-sample/main/gfx/lib/glw

.include <bsd.lib.mk>
.include <bsd.x11.mk>
