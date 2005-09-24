#	$NetBSD: Makefile,v 1.4 2005/09/24 13:51:46 chs Exp $

.include <bsd.own.mk>

LIB=	GLw
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/extras/ogl-sample/main/gfx/lib/glw

SRCS=	GLwDrawA.c GLwM1DrawA.c GLwM2DrawA.c GLwDAUtil.c

LIBDPLIBS=\
	GL	${.CURDIR}/../GL \
	Xt	${.CURDIR}/../Xt \
	X11	${.CURDIR}/../X11/dynamic

INCS=	GLwDrawA.h GLwDrawAP.h GLwMDrawA.h GLwMDrawAP.h
INCSDIR=${X11INCDIR}/GL

CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/${LIB} \
		-I${X11SRCDIR.xc}/extras/ogl-sample/main/gfx/lib/glw

.include <bsd.x11.mk>
.include <bsd.lib.mk>
