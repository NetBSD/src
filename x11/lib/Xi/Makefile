#	$NetBSD: Makefile,v 1.6 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xi
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XAllowDv.c XChgDCtl.c XChgFCtl.c XChgKMap.c XChgKbd.c \
	XChgPnt.c XChgProp.c XCloseDev.c XDevBell.c XExtInt.c \
	XExtToWire.c XGMotion.c XGetBMap.c XGetDCtl.c XGetFCtl.c \
	XGetKMap.c XGetMMap.c XGetProp.c XGetVers.c XGrDvBut.c \
	XGrDvKey.c XGrabDev.c XGtFocus.c XGtSelect.c XListDev.c \
	XOpenDev.c XQueryDv.c XSelect.c XSetBMap.c XSetDVal.c \
	XSetMMap.c XSetMode.c XSndExEv.c XStFocus.c XUngrDev.c \
	XUngrDvB.c XUngrDvK.c

CPPFLAGS+=	${X11FLAGS.THREADS}

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
