#	$NetBSD: Makefile,v 1.15 2016/08/14 03:43:04 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"
.include "Makefile.common"

LIB=		dix

.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c \
		dixutils.c events.c extension.c ffs.c gc.c getevents.c \
		globals.c glyphcurs.c grabs.c initatoms.c main.c pixmap.c \
		privates.c property.c resource.c swaprep.c swapreq.c \
		tables.c window.c stubmain.c \
		enterleave.c ptrveloc.c registry.c selection.c \
		eventconvert.c inpututils.c region.c touch.c


CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/pixman-1
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/xorg
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/miext/sync
CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}
CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX}

.if 0
CPPFLAGS.globals.c=	-DCOMPILEDDEFAULTFONTPATH=\"${X11DEFAULTFONTPATH:ts,}\" \
			-DRGB_DB=\"${X11LIBDIR}/rgb\"
.endif

# XXX make user variables
CPPFLAGS.dispatch.c=	-DVENDOR_NAME="\"The Xorg Foundation\""
CPPFLAGS.main.c=	-DVENDOR_STRING="\"The Xorg Group\"" \
			-DVENDOR_RELEASE=${XORG_VERSION_CURRENT} \
			-DVENDOR_NAME="\"The Xorg Foundation\""

FILES=		protocol.txt
FILESDIR=	${X11ROOTDIR}/lib/xorg

COPTS.dixutils.c=	-Wno-error
COPTS.colormap.c=	-Wno-error
COPTS.cursor.c=		-Wno-error
COPTS.dispatch.c=	-Wno-error
COPTS.dixfonts.c=	-Wno-error
COPTS.events.c=		-Wno-error
COPTS.resource.c=	-Wno-error
COPTS.window.c=		-Wno-error
COPTS.inpututils.c=	-Wno-error

CWARNFLAGS.clang+=	-Wno-switch

.include <bsd.x11.mk>
.include <bsd.lib.mk>
