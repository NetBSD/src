#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:17 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		xinput
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xi

SRCS=		allowev.c chgdctl.c chgfctl.c chgkbd.c chgkmap.c chgprop.c \
		chgptr.c closedev.c devbell.c exevents.c extinit.c getbmap.c \
		getdctl.c getfctl.c getfocus.c getkmap.c getmmap.c getprop.c \
		getselev.c getvers.c grabdev.c grabdevb.c grabdevk.c \
		gtmotion.c listdev.c opendev.c queryst.c selectev.c \
		sendexev.c setbmap.c setdval.c setfocus.c setmmap.c \
		setmode.c stubs.c ungrdev.c ungrdevb.c ungrdevk.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${DESTDIR}/${X11INCDIR}/X11

.include <bsd.x11.mk>
.include <bsd.lib.mk>
