#	$NetBSD: Makefile,v 1.4 2003/09/30 09:49:29 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

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
			-DVENDOR_RELEASE="(((4) * 10000000) + ((    3) * 100000) + ((       0) * 1000) +    0)"

COPTS.gc.c=	-Wno-error	# XXX gcc 3.3
COPTS.window.c=	-Wno-error	# XXX gcc 3.3


.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
