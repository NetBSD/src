#	$NetBSD: Makefile,v 1.4 2004/04/28 14:17:56 lukem Exp $

.include <bsd.own.mk>

PROG=	xclock
SRCS=	xclock.c Clock.c

CPPFLAGS+=-DXRENDER -I${DESTDIR}${X11INCDIR}/freetype2

APPDEFS=XClock.ad XClock-color.ad

BUILDSYMLINKS=  XClk-col.ad XClock-color.ad


LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
