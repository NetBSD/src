# $NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	rac

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	xf86RAC.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg

CPPFLAGS+=	${X11FLAGS.DIX}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
