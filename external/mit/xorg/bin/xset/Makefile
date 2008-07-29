#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xset

CPPFLAGS+=${X11FLAGS.EXTENSION} -DXKB

LDADD+=	-lXmuu -lXt -lSM -lICE -lXxf86misc -lXfontcache -lXext -lX11
DPADD+= ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXXF86MISC} ${LIBXFONTCACHE}
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
