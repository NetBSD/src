#	$NetBSD: Makefile,v 1.7.10.1 2021/06/06 20:30:47 cjep Exp $

.include <bsd.own.mk>

PROG=	xset

CPPFLAGS+=${X11FLAGS.EXTENSION} -DXKB -DHAVE_USLEEP

LDADD+=	-lXxf86misc # XXX
DPADD+= ${LIBXXF86MISC}
LDADD+= ${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XMUU}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
