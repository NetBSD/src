#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:54:13 lukem Exp $

.include <bsd.own.mk>

XTODO=	LIB=cbrt

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

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext

CPPFLAGS.miinitext.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.miscrinit.c=	${X11FLAGS.EXTENSION}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
