#	$NetBSD: Makefile,v 1.7 2009/08/22 02:40:27 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		mi

.PATH:		${X11SRCDIR.xorg-server}/mi
SRCS=		miarc.c mibank.c mibitblt.c mibstore.c \
		micmap.c micursor.c midash.c midispcur.c mieq.c \
		miexpose.c mifillarc.c mifillrct.c mifpolycon.c \
		migc.c miglblt.c mioverlay.c mipointer.c mipoly.c \
		mipolycon.c mipolygen.c mipolypnt.c mipolyrect.c \
		mipolyseg.c mipolytext.c mipolyutil.c mipushpxl.c \
		miregion.c miscrinit.c mispans.c misprite.c \
		mivaltree.c miwideline.c miwindow.c mizerarc.c \
		mizerclip.c mizerline.c

# XXX the order is important since there are headers with the same
#     name but differ depending on location
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/Xi \
		-I${X11SRCDIR.xorg-server}/hw/xnest \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		${X11FLAGS.DIX} \
		-DHAVE_XNEST_CONFIG_H

CPPFLAGS.miscrinit.c=	${X11FLAGS.EXTENSION}

.if 0
.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64" || ${MACHINE} == "mac68k"
SRCS+=  miinitext.c
CPPFLAGS.miinitext.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.miinitext.c+=   -UXF86MISC -UXINPUT -UXF86VIDMODE -UXFreeXDGA
CPPFLAGS.miscrinit.c+=   -UXF86MISC -UXINPUT -UXF86VIDMODE -UXFreeXDGA
.endif
.endif

COPTS.micmap.c+=	-Wno-error
COPTS.miexpose.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
