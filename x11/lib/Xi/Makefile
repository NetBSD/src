#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:14 lukem Exp $

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

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
