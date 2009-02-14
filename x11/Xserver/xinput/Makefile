#	$NetBSD: Makefile,v 1.12 2009/02/14 13:48:17 abs Exp $

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
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/X11

.if ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE} == "amiga" || \
    ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE} == "acorn32" || \
    ${MACHINE} == "pmax" || \
    ${MACHINE} == "sun3" || \
    ${MACHINE} == "dreamcast" || \
    ${MACHINE} == "ews4800mips" || \
    ${MACHINE} == "newsmips" || \
    ${MACHINE_ARCH} == "vax"
CPPFLAGS+=	-DXINPUT
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
