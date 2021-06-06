#	$NetBSD: Makefile,v 1.4.10.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

PROG=	xditview
SRCS=	xditview.c Dvi.c draw.c font.c lex.c page.c parse.c \
	XFontName.c DviChar.c

CPPFLAGS+=	-DHAS_MKSTEMP
CPPFLAGS+=	-DUSE_XFT -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=Xditview Xditview-chrtr

LDADD+=	${XLIBLDADD_XFT} ${XLIBLDADD_XAW} -lm
DPADD+=	${XLIBDPADD_XFT} ${XLIBDPADD_XAW} ${LIBM}

FILES=	ldblarrow rdblarrow
FILESDIR=${X11INCDIR}/X11/bitmaps

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
