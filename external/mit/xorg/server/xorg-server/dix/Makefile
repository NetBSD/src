#	$NetBSD: Makefile,v 1.7 2009/06/11 05:43:15 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"
.include "Makefile.common"

LIB=		dix


.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c \
		dixutils.c events.c extension.c ffs.c gc.c getevents.c \
		globals.c glyphcurs.c grabs.c initatoms.c main.c pixmap.c \
		privates.c property.c resource.c swaprep.c swapreq.c \
		tables.c window.c \
		deprecated.c enterleave.c ptrveloc.c registry.c selection.c


CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/pixman-1
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/xorg
CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}
CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX}

CPPFLAGS.globals.c=	-DCOMPILEDDEFAULTFONTPATH=\"${X11DEFAULTFONTPATH:ts,}\" \
			-DRGB_DB=\"${X11LIBDIR}/rgb\"

# XXX make user variables
CPPFLAGS.main.c=	-DVENDOR_STRING="\"The Xorg Group\"" \
			-DVENDOR_RELEASE=${XORG_VERSION_CURRENT} \
			-DVENDOR_NAME="\"The Xorg Foundation\""

COPTS.dixutils.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
