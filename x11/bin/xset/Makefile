#	$NetBSD: Makefile,v 1.3 2003/09/13 21:50:54 lukem Exp $

.include <bsd.own.mk>

PROG=	xset

CPPFLAGS+=${X11FLAGS.EXTENSION} -DXKB

LDADD+=	-lXmuu -lXt -lSM -lICE -lXxf86misc -lXfontcache -lXext -lX11
DPADD+= ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXXF86MISC} ${LIBXFONTCACHE}
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
