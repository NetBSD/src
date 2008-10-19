# $NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:20 haad Exp $

#XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	ramdac

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	xf86RamDac.c xf86RamDacCmap.c \
	xf86Cursor.c xf86HWCurs.c IBM.c BT.c TI.c \
	xf86BitOrder.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg

CPPFLAGS+=	${X11FLAGS.DIX}

CPPFLAGS.xf86BitOrder.c=	-DRAMDAC_MODULE

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
