#	$NetBSD: Makefile,v 1.4.2.1 2004/04/29 06:08:26 jmc Exp $

.include <bsd.own.mk>

PROG=	xlogo
SRCS=	xlogo.c Logo.c RenderLogo.c

CPPFLAGS+=	-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2

APPDEFS=XLogo.ad XLogo-color.ad

BUILDSYMLINKS=	XLogo-co.ad XLogo-color.ad

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
