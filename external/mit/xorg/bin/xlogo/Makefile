#	$NetBSD: Makefile,v 1.3.26.1 2018/05/21 04:35:53 pgoyette Exp $

.include <bsd.own.mk>

PROG=	xlogo
SRCS=	xlogo.c Logo.c RenderLogo.c

CPPFLAGS+=	-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=XLogo XLogo-color

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
