#	$NetBSD: Makefile,v 1.1 2005/03/21 00:19:19 rtr Exp $

.include "../../../Makefile.serverlib"

LIB=		dmxinput

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/dmx/input
SRCS=		dmxdummy.c dmxbackend.c dmxconsole.c dmxcommon.c \
		dmxinputinit.c dmxarg.c dmxsigio.c dmxevents.c \
		dmxxinput.c dmxmotion.c dmxeq.c dmxmap.c ChkNotMaskEv.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx/input \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/Xi \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.lib.mk>
