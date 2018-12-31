#	$NetBSD: Makefile,v 1.9 2018/12/31 11:14:06 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            randr

.PATH:          ${X11SRCDIR.xorg-server}/randr
SRCS=  \
		randr.c \
		rrcrtc.c \
		rrdispatch.c \
		rrinfo.c \
		rrlease.c \
		rrmode.c \
		rrmonitor.c \
		rroutput.c \
		rrpointer.c \
		rrproperty.c \
		rrprovider.c \
		rrproviderproperty.c \
		rrscreen.c \
		rrsdispatch.c \
		rrtransform.c \
		rrxinerama.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

COPTS.randr.c+=		-Wno-error
COPTS.rrcrtc.c+=	-Wno-error
COPTS.rrdispatch.c+=	-Wno-error
COPTS.rrmode.c+=	-Wno-error
COPTS.rroutput.c+=	-Wno-error
COPTS.rrproperty.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
