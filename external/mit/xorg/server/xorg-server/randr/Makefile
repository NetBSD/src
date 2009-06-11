#	$NetBSD: Makefile,v 1.3 2009/06/11 05:43:19 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            randr

.PATH:          ${X11SRCDIR.xorg-server}/randr
SRCS=           mirandr.c randr.c randrstr.h rrcrtc.c rrdispatch.c \
		rrinfo.c rrmode.c rroutput.c rrpointer.c rrproperty.c \
		rrscreen.c rrsdispatch.c rrtransform.c
SRCS+=		rrxinerama.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
