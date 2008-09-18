#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:48 wrstuden Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            randr

.PATH:          ${X11SRCDIR.xorg-server}/randr
SRCS=           mirandr.c randr.c randrstr.h rrcrtc.c rrdispatch.c \
		rrinfo.c rrmode.c rroutput.c rrpointer.c rrproperty.c \
		rrscreen.c rrsdispatch.c
SRCS+=		rrxinerama.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
