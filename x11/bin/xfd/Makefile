#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:19 lukem Exp $

.include <bsd.own.mk>

PROG=	xfd
SRCS=	xfd.c grid.c

CPPFLAGS+=-DXRENDER -I${DESTDIR}/${X11INCDIR}/freetype2

APPDEFS=Xfd.ad

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
