#	$NetBSD: Makefile,v 1.8.4.1 2004/11/15 09:12:43 rtr Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		mi
.PATH:		${X11SRCDIR.xc}/programs/Xserver/mi

SRCS=		miarc.c mibank.c mibitblt.c mibstore.c miclipn.c \
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
CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/damage \
		-I${X11SRCDIR.xc}/include/extensions

CPPFLAGS.miinitext.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.miscrinit.c=	${X11FLAGS.EXTENSION}

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64" || ${MACHINE} == "mac68k"
SRCS+=  miinitext.c
CPPFLAGS.miinitext.c+=   -UXF86MISC -UXINPUT -UXF86VIDMODE -UXFreeXDGA
CPPFLAGS.miscrinit.c+=   -UXF86MISC -UXINPUT -UXF86VIDMODE -UXFreeXDGA
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
