#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:17 haad Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		Xi

.PATH:		${X11SRCDIR.xorg-server}/${LIB}

SRCS=		allowev.c chgdctl.c chgfctl.c chgkbd.c chgkmap.c chgprop.c \
		chgptr.c closedev.c devbell.c exevents.c extinit.c \
		getbmap.c getdctl.c getfctl.c getfocus.c getkmap.c \
		getmmap.c getprop.c getselev.c getvers.c grabdev.c \
		grabdevb.c grabdevk.c gtmotion.c listdev.c opendev.c \
		queryst.c selectev.c sendexev.c setbmap.c setdval.c \
		setfocus.c setmmap.c setmode.c ungrdev.c ungrdevb.c \
		ungrdevk.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
