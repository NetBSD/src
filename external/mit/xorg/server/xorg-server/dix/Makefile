#	$NetBSD: Makefile,v 1.1 2008/08/02 04:32:00 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		dix


.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c \
		dixutils.c events.c extension.c ffs.c gc.c getevents.c \
		globals.c glyphcurs.c grabs.c initatoms.c main.c pixmap.c \
		privates.c property.c resource.c swaprep.c swapreq.c \
		tables.c window.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/pixman-1
CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}
CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX}

CPPFLAGS.globals.c=	-DCOMPILEDDEFAULTFONTPATH=\"${X11FONTDIR}/misc/,${X11FONTDIR}/Speedo/,${X11FONTDIR}/Type1/,${X11FONTDIR}/CID/,${X11FONTDIR}/75dpi/,${X11FONTDIR}/100dpi/\" \
			-DRGB_DB=\"${X11LIBDIR}/rgb\"

# XXX make user variables
CPPFLAGS.main.c=	-DVENDOR_STRING="\"The Xorg Group\"" \
			-DVENDOR_RELEASE="${XVERSION}" \
			-DVENDOR_NAME="\"The Xorg Foundation\""

COPTS.dixutils.c=	-Wno-error

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.lib.mk>

SUBDIR=	xpstubs
.include <bsd.subdir.mk>
