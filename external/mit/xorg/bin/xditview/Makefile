#	$NetBSD: Makefile,v 1.5 2022/10/16 23:38:02 mrg Exp $

.include <bsd.own.mk>

PROG=	xditview
SRCS=	xditview.c Dvi.c draw.c font.c lex.c page.c parse.c \
	XFontName.c DviChar.c

CPPFLAGS+=	-D_CONST_X_STRING
CPPFLAGS+=	-DHAS_MKSTEMP
CPPFLAGS+=	-DUSE_XFT -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=Xditview Xditview-chrtr

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

FILES=	ldblarrow rdblarrow
FILESDIR=${X11INCDIR}/X11/bitmaps

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
