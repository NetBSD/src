# $NetBSD: Makefile,v 1.2 2008/08/09 01:24:16 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

#SHLIB_MAJOR=	0
LIB=	extmod

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods/extmod
SRCS=	xf86dga.c xf86dga2.c \
	xvmod.c \
	xf86misc.c \
	xf86vmode.c \
	modinit.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/afb \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/mfb \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/cfb \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dbe \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/hw/xfree86/loader \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/miext/shadow \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.DIX}

# XXX
CPPFLAGS.xf86RandR12.c=	-Wno-error

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
