# $NetBSD: Makefile,v 1.9 2022/07/15 04:18:55 mrg Exp $

#XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	ramdac

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	xf86CursorRD.c xf86HWCurs.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	${X11FLAGS.DIX}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
