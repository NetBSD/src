#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:49 lukem Exp $

.include <bsd.own.mk>

PROG=	xditview
SRCS=	xditview.c Dvi.c draw.c font.c lex.c page.c parse.c \
	XFontName.c DviChar.c

CPPFLAGS+=-DHAS_MKSTEMP -DUSE_XFT -I${DESTDIR}/${X11INCDIR}/freetype2

APPDEFS=Xditview.ad Xditview-chrtr.ad

BUILDSYMLINKS=	Xdit-chrtr.ad Xditview-chrtr.ad

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
