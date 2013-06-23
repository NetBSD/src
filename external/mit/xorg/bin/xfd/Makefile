#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xfd
SRCS=	xfd.c grid.c

CPPFLAGS+=	-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2

APPDEFS=Xfd

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
