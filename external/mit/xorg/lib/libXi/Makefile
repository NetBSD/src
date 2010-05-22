#	$NetBSD: Makefile,v 1.5 2010/05/22 02:41:33 mrg Exp $

.include <bsd.own.mk>

LIB=	Xi
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	XAllowDv.c XChgDCtl.c XChgFCtl.c XChgKMap.c XChgKbd.c \
	XChgPnt.c XChgProp.c XCloseDev.c XDevBell.c XExtInt.c \
	XExtToWire.c XGMotion.c XGetBMap.c XGetDCtl.c XGetFCtl.c \
	XGetKMap.c XGetMMap.c XGetProp.c XGetVers.c XGrDvBut.c \
	XGrDvKey.c XGrabDev.c XGtFocus.c XGtSelect.c XListDev.c \
	XOpenDev.c XQueryDv.c XSelect.c XSetBMap.c XSetDVal.c \
	XSetMMap.c XSetMode.c XSndExEv.c XStFocus.c XUngrDev.c \
	XUngrDvB.c XUngrDvK.c XChDProp.c XGetDProp.c XDelDProp.c \
	XListDProp.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

INCS=	XInput.h \
	XInput2.h

INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

COPTS.XExtInt.c+=	-Wno-error	# XXX
COPTS.XSndExEv.c+=	-Wno-error	# XXX

.include <bsd.x11.mk>
.include <bsd.lib.mk>
