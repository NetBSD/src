#	$NetBSD: Makefile,v 1.8 2004/02/07 06:52:05 lukem Exp $

.include "../Makefile.serverlib"

LIB=		dix
.PATH:		${X11SRCDIR.xc}/programs/Xserver/dix

SUBDIR=		xpstubs

SRCS=		atom.c colormap.c cursor.c devices.c dispatch.c dixutils.c \
		events.c extension.c gc.c globals.c glyphcurs.c grabs.c \
		main.c property.c resource.c swaprep.c swapreq.c tables.c \
		window.c initatoms.c dixfonts.c privates.c pixmap.c


CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/lbx


CPPFLAGS.globals.c=	-DCOMPILEDDEFAULTFONTPATH=\"${X11FONTDIR}/misc/,${X11FONTDIR}/Speedo/,${X11FONTDIR}/Type1/,${X11FONTDIR}/CID/,${X11FONTDIR}/75dpi/,${X11FONTDIR}/100dpi/\" \
			-DRGB_DB=\"${X11LIBDIR}/rgb\"

CPPFLAGS.main.c=	-DVENDOR_STRING="\"The XFree86 Project, Inc\"" \
			-DVENDOR_RELEASE="${XVERSION}"

COPTS.gc.c=	-Wno-error	# XXX gcc 3.3
COPTS.window.c=	-Wno-error	# XXX gcc 3.3


.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
