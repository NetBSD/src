#	$NetBSD: Makefile,v 1.4.10.1 2021/06/06 20:30:47 cjep Exp $

.include <bsd.own.mk>

PROG=	xlogo
SRCS=	xlogo.c Logo.c RenderLogo.c

CPPFLAGS+=	-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=XLogo XLogo-color

LDADD+=	${XLIBLDADD_XFT} ${XLIBLDADD_XAW} -lm
DPADD+=	${XLIBDPADD_XFT} ${XLIBDPADD_XAW} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
