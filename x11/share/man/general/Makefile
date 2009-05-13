#	$NetBSD: Makefile,v 1.6.26.1 2009/05/13 19:20:50 jym Exp $

.include <bsd.own.mk>

MAN=	X.7 XStandards.7 Xsecurity.7

BUILDSYMLINKS=	Standards.man	XStandards.man	\
		security.man	Xsecurity.man

.PATH:  ${X11SRCDIR.xc}/doc/man/general

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "cats" || \
    ${MACHINE} == "shark" || ${MACHINE_ARCH} == "i386" || \
    ${MACHINE} == "atari" || ${MACHINE} == "macppc"
MAN+=	XF86VidMode.3
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/doc/man

BUILDSYMLINKS+=	XF86VM.man XF86VidMode.man

MLINKS+=XF86VidMode.3 XF86VidModeQueryExtension.3 \
	XF86VidMode.3 XF86VidModeQueryVersion.3 \
	XF86VidMode.3 XF86VidModeGetModeLine.3 \
	XF86VidMode.3 XF86VidModeGetAllModeLines.3 \
	XF86VidMode.3 XF86VidModeDeleteModeLine.3 \
	XF86VidMode.3 XF86VidModeSwitchMode.3 \
	XF86VidMode.3 XF86VidModeLockModeSwitch.3 \
	XF86VidMode.3 XF86VidModeGetMonitor.3 \
	XF86VidMode.3 XF86VidModeGetViewPort.3 \
	XF86VidMode.3 XF86VidModeValidateModeLine.3 \
	XF86VidMode.3 XF86VidModeSetClientVersion.3 \
	XF86VidMode.3 XF86VidModeGetDotClocks.3 \
	XF86VidMode.3 XF86VidModeGetGamma.3 \
	XF86VidMode.3 XF86VidModeSetGamma.3 \
	XF86VidMode.3 XF86VidModeSetGammaRamp.3 \
	XF86VidMode.3 XF86VidModeGetGammaRamp.3 \
	XF86VidMode.3 XF86VidModeGetGammaRampSize.3 \
	XF86VidMode.3 XF86VidModeGetPermissions.3 \
	XF86VidMode.3 XF86VidModeModModeLine.3 \
	XF86VidMode.3 XF86VidModeSetViewPort.3 \
	XF86VidMode.3 XF86VidModeSwitchToMode.3
.endif

.include <bsd.x11.mk>
.include <bsd.man.mk>
