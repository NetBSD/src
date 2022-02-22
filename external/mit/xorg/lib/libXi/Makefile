#	$NetBSD: Makefile,v 1.14 2022/02/22 22:49:39 uwe Exp $

.include <bsd.own.mk>

LIB=	Xi
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	XIAllowEvents.c \
	XIBarrier.c \
	XIGrabDevice.c \
	XIQueryVersion.c \
	XIQueryDevice.c \
	XISetDevFocus.c \
	XIGetDevFocus.c \
	XIPassiveGrab.c \
	XIProperties.c \
	XISelEv.c \
	XISetCPtr.c \
	XIWarpPointer.c \
	XIHierarchy.c \
	XIDefineCursor.c \
	XIQueryPointer.c
SRCS+=	XAllowDv.c \
	XChDProp.c \
	XChgDCtl.c \
	XChgFCtl.c \
	XChgKbd.c \
	XChgKMap.c \
	XChgPnt.c \
	XChgProp.c \
	XCloseDev.c \
	XDelDProp.c \
	XDevBell.c \
	XExtToWire.c \
	XGetBMap.c \
	XGetCPtr.c \
	XGetDCtl.c \
	XGetDProp.c \
	XGetFCtl.c \
	XGetKMap.c \
	XGetMMap.c \
	XGetProp.c \
	XGetVers.c \
	XGMotion.c \
	XGrabDev.c \
	XGrDvBut.c \
	XGrDvKey.c \
	XGtFocus.c \
	XGtSelect.c \
	XListDev.c \
	XListDProp.c \
	XOpenDev.c \
	XQueryDv.c \
	XSelect.c \
	XSetBMap.c \
	XSetDVal.c \
	XSetMMap.c \
	XSetMode.c \
	XSndExEv.c \
	XStFocus.c \
	XUngrDev.c \
	XUngrDvB.c \
	XUngrDvK.c \
	XExtInt.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DHAVE__XEATDATAWORDS

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

INCS=	XInput.h \
	XInput2.h

INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

COPTS.XExtInt.c+=	-Wno-error	# XXX
COPTS.XSndExEv.c+=	-Wno-error	# XXX

CWARNFLAGS.clang+=	-Wno-pointer-sign

# X Input v1 man pages
MAN +=		XAllowDeviceEvents.3
MAN +=		XChangeDeviceDontPropagateList.3
MAN +=		XChangeKeyboardDevice.3
MAN +=		XChangePointerDevice.3
MAN +=		XDeviceBell.3
MAN +=		XGetDeviceControl.3
MAN +=		XGetDeviceKeyMapping.3
MAN +=		XGetDeviceModifierMapping.3
MAN +=		XGetDeviceMotionEvents.3
MAN +=		XGetDeviceProperty.3
MAN +=		XGetExtensionVersion.3
MAN +=		XGetFeedbackControl.3
MAN +=		XGrabDeviceButton.3
MAN +=		XGrabDeviceKey.3
MAN +=		XGrabDevice.3
MAN +=		XListDeviceProperties.3
MAN +=		XListInputDevices.3
MAN +=		XOpenDevice.3
MAN +=		XQueryDeviceState.3
MAN +=		XSelectExtensionEvent.3
MAN +=		XSendExtensionEvent.3
MAN +=		XSetDeviceButtonMapping.3
MAN +=		XSetDeviceFocus.3
MAN +=		XSetDeviceMode.3
MAN +=		XSetDeviceValuators.3

MLINKS +=	XChangeDeviceDontPropagateList.3 XGetDeviceDontPropagateList.3
MLINKS +=	XGetDeviceControl.3		XChangeDeviceControl.3
MLINKS +=	XGetDeviceKeyMapping.3		XChangeDeviceKeyMapping.3
MLINKS +=	XGetDeviceModifierMapping.3	XSetDeviceModifierMapping.3
MLINKS +=	XGetDeviceMotionEvents.3	XDeviceTimeCoord.3
MLINKS +=	XGetDeviceProperty.3		XChangeDeviceProperty.3
MLINKS +=	XGetDeviceProperty.3		XDeleteDeviceProperty.3
MLINKS +=	XGetFeedbackControl.3		XChangeFeedbackControl.3
MLINKS +=	XGrabDevice.3			XUngrabDevice.3
MLINKS +=	XGrabDeviceButton.3		XUngrabDeviceButton.3
MLINKS +=	XGrabDeviceKey.3		XUngrabDeviceKey.3
MLINKS +=	XListInputDevices.3		XFreeDeviceList.3
MLINKS +=	XOpenDevice.3			XCloseDevice.3
MLINKS +=	XSelectExtensionEvent.3		XGetSelectedExtensionEvents.3
MLINKS +=	XSetDeviceButtonMapping.3	XGetDeviceButtonMapping.3
MLINKS +=	XSetDeviceFocus.3		XGetDeviceFocus.3


# X Input v2 man pages
MAN +=		XIAllowEvents.3
MAN +=		XIBarrierReleasePointer.3
MAN +=		XIChangeHierarchy.3
MAN +=		XIChangeProperty.3
MAN +=		XIDefineCursor.3
MAN +=		XIGrabButton.3
MAN +=		XIGrabDevice.3
MAN +=		XIGrabEnter.3
MAN +=		XIListProperties.3
MAN +=		XIQueryDevice.3
MAN +=		XIQueryPointer.3
MAN +=		XIQueryVersion.3
MAN +=		XISelectEvents.3
MAN +=		XISetClientPointer.3
MAN +=		XISetFocus.3
MAN +=		XIWarpPointer.3

MLINKS +=	XIBarrierReleasePointer.3	XIBarrierReleasePointers.3
MLINKS +=	XIChangeProperty.3		XIDeleteProperty.3
MLINKS +=	XIChangeProperty.3		XIGetProperty.3
MLINKS +=	XIDefineCursor.3		XIUndefineCursor.3
MLINKS +=	XIGrabButton.3			XIGrabKeycode.3
MLINKS +=	XIGrabButton.3			XIGrabTouchBegin.3
MLINKS +=	XIGrabButton.3			XIUngrabButton.3
MLINKS +=	XIGrabButton.3			XIUngrabKeycode.3
MLINKS +=	XIGrabButton.3			XIUngrabTouchBegin.3
MLINKS +=	XIGrabDevice.3			XIUngrabDevice.3
MLINKS +=	XIGrabEnter.3			XIGrabFocusIn.3
MLINKS +=	XIGrabEnter.3			XIUngrabEnter.3
MLINKS +=	XIGrabEnter.3			XIUngrabFocusIn.3
MLINKS +=	XIQueryDevice.3			XIFreeDeviceInfo.3
MLINKS +=	XISelectEvents.3		XIGetSelectedEvents.3
MLINKS +=	XISetClientPointer.3		XIGetClientPointer.3
MLINKS +=	XISetFocus.3			XIGetFocus.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
