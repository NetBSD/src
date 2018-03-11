#	$NetBSD: Makefile,v 1.7 2018/03/11 08:46:31 mrg Exp $

.include <bsd.own.mk>

PROG=	xset

CPPFLAGS+=${X11FLAGS.EXTENSION} -DXKB -DHAVE_USLEEP

LDADD+=	-lXmuu -lXt -lSM -lICE -lXxf86misc -lXext -lX11
DPADD+= ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXXF86MISC}
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
