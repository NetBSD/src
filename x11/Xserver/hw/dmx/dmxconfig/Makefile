#	$NetBSD: Makefile,v 1.1 2005/03/21 00:19:19 rtr Exp $

.include "../../../Makefile.serverlib"

LIB=		dmxconfig

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/dmx/config
SRCS=		scanner.l parser.y parser.c scanner.c dmxparse.c dmxprint.c dmxcompat.c dmxconfig.c

YHEADER=	# defined

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx/config \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I. \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.lib.mk>
