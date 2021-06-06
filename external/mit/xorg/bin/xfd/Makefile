#	$NetBSD: Makefile,v 1.4.10.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

PROG=	xfd
SRCS=	xfd.c grid.c

CPPFLAGS+=	-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=Xfd

LDADD+=	${XLIBLDADD_XAW} ${XLIBLDADD_XFT} -lm
DPADD+= ${XLIBDPADD_XAW} ${XLIBDPADD_XFT} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
