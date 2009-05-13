#	$NetBSD: Makefile,v 1.3.8.1 2009/05/13 18:53:26 jym Exp $

.include <bsd.own.mk>

LIB=	Xi
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man

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

MAN=	XAllowDeviceEvents.3 XChangeDeviceDontPropagateList.3 \
	XChangeKeyboardDevice.3 XChangePointerDevice.3 XDeviceBell.3 \
	XGetDeviceControl.3 XGetDeviceKeyMapping.3 \
	XGetDeviceModifierMapping.3 XGetDeviceMotionEvents.3 \
	XGetExtensionVersion.3 XGetFeedbackControl.3 XGrabDevice.3 \
	XGrabDeviceButton.3 XGrabDeviceKey.3 XListInputDevices.3 \
	XOpenDevice.3 XQueryDeviceState.3 XSelectExtensionEvent.3 \
	XSendExtensionEvent.3 XSetDeviceButtonMapping.3 XSetDeviceFocus.3 \
	XSetDeviceMode.3 XSetDeviceValuators.3 \
	XGetDeviceProperty.3 XListDeviceProperties.3

MLINKS=	XGetDeviceControl.3 XChangeDeviceControl.3 \
	XGetFeedbackControl.3 XChangeFeedbackControl.3 \
	XGetDeviceKeyMapping.3 XChangeDeviceKeyMapping.3 \
	XGetDeviceModifierMapping.3 XSetDeviceModifierMapping.3 \
	XChangeDeviceDontPropagateList.3 XGetDeviceDontPropagateList.3 \
	XGetDeviceMotionEvents.3 XDeviceTimeCoord.3 \
	XGrabDeviceButton.3 XUngrabDeviceButton.3 \
	XGrabDeviceKey.3 XUngrabDeviceKey.3 \
	XGrabDevice.3 XUngrabDevice.3 \
	XListInputDevices.3 XFreeDeviceList.3 \
	XOpenDevice.3 XCloseDevice.3 \
	XSetDeviceButtonMapping.3 XGetDeviceButtonMapping.3 \
	XSetDeviceFocus.3 XGetDeviceFocus.3 \
	XSelectExtensionEvent.3 XGetSelectedExtensionEvents.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
