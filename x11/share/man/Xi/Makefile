#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:06 lukem Exp $

.include <bsd.own.mk>

MAN=	XAllowDeviceEvents.3 XGetDeviceControl.3 XGetFeedbackControl.3 \
	XGetDeviceKeyMapping.3 XGetDeviceModifierMapping.3 \
	XChangeDeviceDontPropagateList.3 XChangeKeyboardDevice.3 \
	XChangePointerDevice.3 XDeviceBell.3 XGetDeviceMotionEvents.3 \
	XGetExtensionVersion.3 XGrabDeviceButton.3 XGrabDeviceKey.3 \
	XGrabDevice.3 XListInputDevices.3 XOpenDevice.3 \
	XQueryDeviceState.3 XSendExtensionEvent.3 XSetDeviceButtonMapping.3 \
	XSetDeviceFocus.3 XSelectExtensionEvent.3 XSetDeviceValuators.3 \
	XSetDeviceMode.3

MLINKS=	XChangeDeviceDontPropagateList.3 XGetDeviceDontPropagateList.3 \
	XGetDeviceControl.3 XChangeDeviceControl.3 \
	XGetDeviceKeyMapping.3 XChangeDeviceKeyMapping.3 \
	XGetDeviceModifierMapping.3 XSetDeviceModifierMapping.3 \
	XGetDeviceMotionEvents.3 XDeviceTimeCoord.3 \
	XGetFeedbackControl.3 XChangeFeedbackControl.3 \
	XGrabDevice.3 XUngrabDevice.3 \
	XGrabDeviceButton.3 XUngrabDeviceButton.3 \
	XGrabDeviceKey.3 XUngrabDeviceKey.3 \
	XListInputDevices.3 XFreeDeviceList.3 \
	XOpenDevice.3 XCloseDevice.3 \
	XSelectExtensionEvent.3 XGetSelectedExtensionEvents.3 \
	XSetDeviceButtonMapping.3 XGetDeviceButtonMapping.3 \
	XSetDeviceFocus.3 XGetDeviceFocus.3

LNFILES=XAllDvEv.man XAllowDeviceEvents.man \
	XChDCtl.man XGetDeviceControl.man \
	XChFCtl.man XGetFeedbackControl.man \
	XChKMap.man XGetDeviceKeyMapping.man \
	XChMMap.man XGetDeviceModifierMapping.man \
	XChProp.man XChangeDeviceDontPropagateList.man \
	XChgKbd.man XChangeKeyboardDevice.man \
	XChgPtr.man XChangePointerDevice.man \
	XDevBell.man XDeviceBell.man \
	XGetDvMo.man XGetDeviceMotionEvents.man \
	XGetExtV.man XGetExtensionVersion.man \
	XGrDvBut.man XGrabDeviceButton.man \
	XGrDvKey.man XGrabDeviceKey.man \
	XGrabDev.man XGrabDevice.man \
	XListDev.man XListInputDevices.man \
	XOpenDev.man XOpenDevice.man \
	XQueryDv.man XQueryDeviceState.man \
	XSExEvnt.man XSendExtensionEvent.man \
	XSeBMap.man XSetDeviceButtonMapping.man \
	XSeDvFoc.man XSetDeviceFocus.man \
	XSelExtEv.man XSelectExtensionEvent.man \
	XSetDVal.man XSetDeviceValuators.man \
	XSetMode.man XSetDeviceMode.man

.PATH:  ${X11SRCDIR.xc}/doc/man/Xi

.include <bsd.x11.mk>
.include <bsd.man.mk>
