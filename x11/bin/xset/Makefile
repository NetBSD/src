#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:35 lukem Exp $

.include <bsd.own.mk>

PROG=	xset

CPPFLAGS+=${X11FLAGS.EXTENSION} -DXKB

LDADD+=	-lXmuu -lXxf86misc -lXfontcache -lXext -lX11
DPADD+= ${LIBXMUU} ${LIBXXF86MISC} ${LIBXFONTCACHE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
