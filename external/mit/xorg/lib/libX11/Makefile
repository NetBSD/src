#	$NetBSD: Makefile,v 1.16 2022/05/01 13:45:46 uwe Exp $

.include <bsd.own.mk>

SUBDIR=		dynamic .WAIT libX11-xcb static \
		extensions

INCS=		ImUtil.h XKBlib.h Xcms.h Xlib-xcb.h Xlib.h XlibConf.h \
		Xlibint.h Xlocale.h Xregion.h Xresource.h Xutil.h \
		Xcmsint.h Xlcint.h \
		cursorfont.h
INCSDIR=	${X11INCDIR}/X11

MAN=		AllPlanes.3 BlackPixelOfScreen.3 DisplayOfCCC.3 \
		ImageByteOrder.3 IsCursorKey.3 XAddConnectionWatch.3 \
		XAddHost.3 XAllocClassHint.3 XAllocColor.3 \
		XAllocIconSize.3 XAllocSizeHints.3 \
		XAllocStandardColormap.3 XAllocWMHints.3 \
		XAllowEvents.3 XAnyEvent.3 XButtonEvent.3 \
		XChangeKeyboardControl.3 XChangeKeyboardMapping.3 \
		XChangePointerControl.3 XChangeSaveSet.3 \
		XChangeWindowAttributes.3 XCirculateEvent.3 \
		XCirculateRequestEvent.3 XClearArea.3 \
		XClientMessageEvent.3 XColormapEvent.3 \
		XConfigureEvent.3 XConfigureRequestEvent.3 \
		XConfigureWindow.3 XCopyArea.3 XCreateColormap.3 \
		XCreateFontCursor.3 XCreateFontSet.3 XCreateGC.3 \
		XCreateIC.3 XCreateOC.3 XCreatePixmap.3 \
		XCreateRegion.3 XCreateWindow.3 XCreateWindowEvent.3 \
		XCrossingEvent.3 XDefineCursor.3 XDestroyWindow.3 \
		XDestroyWindowEvent.3 XDrawArc.3 XDrawImageString.3 \
		XDrawLine.3 XDrawPoint.3 XDrawRectangle.3 \
		XDrawString.3 XDrawText.3 XEmptyRegion.3 \
		XErrorEvent.3 XExposeEvent.3 XExtentsOfFontSet.3 \
		XFillRectangle.3 XFilterEvent.3 XFlush.3 \
		XFocusChangeEvent.3 XFontSetExtents.3 \
		XFontsOfFontSet.3 XFree.3 XGetVisualInfo.3 \
		XGetWindowAttributes.3 XGetWindowProperty.3 \
		XGetXCBConnection.3 XGrabButton.3 XGrabKey.3 \
		XGrabKeyboard.3 XGrabPointer.3 XGrabServer.3 \
		XGraphicsExposeEvent.3 XGravityEvent.3 \
		XIconifyWindow.3 XIfEvent.3 XInitImage.3 \
		XInitThreads.3 XInstallColormap.3 XInternAtom.3 \
		XIntersectRegion.3 XKeymapEvent.3 XListFonts.3 \
		XLoadFont.3 XLookupKeysym.3 XMapEvent.3 \
		XMapRequestEvent.3 XMapWindow.3 XNextEvent.3 XNoOp.3 \
		XOpenDisplay.3 XOpenIM.3 XOpenOM.3 XParseGeometry.3 \
		XPolygonRegion.3 XPropertyEvent.3 XPutBackEvent.3 \
		XPutImage.3 XQueryBestSize.3 XQueryColor.3 \
		XQueryExtension.3 XQueryPointer.3 XQueryTree.3 \
		XRaiseWindow.3 XReadBitmapFile.3 XRecolorCursor.3 \
		XReparentEvent.3 XReparentWindow.3 \
		XResizeRequestEvent.3 XResourceManagerString.3 \
		XSaveContext.3 XSelectInput.3 XSelectionClearEvent.3 \
		XSelectionEvent.3 XSelectionRequestEvent.3 \
		XSendEvent.3 XSetArcMode.3 XSetClipOrigin.3 \
		XSetCloseDownMode.3 XSetCommand.3 XSetErrorHandler.3 \
		XSetEventQueueOwner.3 XSetFillStyle.3 XSetFont.3 \
		XSetFontPath.3 XSetICFocus.3 XSetICValues.3 \
		XSetInputFocus.3 XSetLineAttributes.3 \
		XSetPointerMapping.3 XSetScreenSaver.3 \
		XSetSelectionOwner.3 XSetState.3 XSetTextProperty.3 \
		XSetTile.3 XSetTransientForHint.3 \
		XSetWMClientMachine.3 XSetWMColormapWindows.3 \
		XSetWMIconName.3 XSetWMName.3 XSetWMProperties.3 \
		XSetWMProtocols.3 XStoreBytes.3 XStoreColors.3 \
		XStringListToTextProperty.3 XStringToKeysym.3 \
		XSupportsLocale.3 XSynchronize.3 XTextExtents.3 \
		XTextWidth.3 XTranslateCoordinates.3 XUnmapEvent.3 \
		XUnmapWindow.3 XVaCreateNestedList.3 \
		XVisibilityEvent.3 XWarpPointer.3 XcmsAllocColor.3 \
		XcmsCCCOfColormap.3 XcmsCIELabQueryMaxC.3 \
		XcmsCIELuvQueryMaxC.3 XcmsColor.3 XcmsConvertColors.3 \
		XcmsCreateCCC.3 XcmsDefaultCCC.3 XcmsQueryBlack.3 \
		XcmsQueryColor.3 XcmsSetWhitePoint.3 XcmsStoreColor.3 \
		XcmsTekHVCQueryMaxC.3 XmbDrawImageString.3 \
		XmbDrawString.3 XmbDrawText.3 XmbLookupString.3 \
		XmbResetIC.3 XmbTextEscapement.3 XmbTextExtents.3 \
		XmbTextListToTextProperty.3 XmbTextPerCharExtents.3 \
		XrmEnumerateDatabase.3 XrmGetFileDatabase.3 \
		XrmGetResource.3 XrmInitialize.3 XrmMergeDatabases.3 \
		XrmPutResource.3 XrmUniqueQuark.3
# XGetEventData.man

USETBL=		yes

FILES=		XErrorDB XKeysymDB Xcms.txt
FILESDIR=	${X11LIBDIR}

.PATH:		${X11SRCDIR.X11}
.PATH:		${X11SRCDIR.X11}/include/X11
.PATH:		${X11SRCDIR.X11}/man
.PATH:		${X11SRCDIR.X11}/src
.PATH:		${X11SRCDIR.X11}/src/xcms
.PATH:		${X11SRCDIR.X11}/src/xlibi18n

XlibConf.h: XlibConf.h.in
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/#undef XTHREADS/#define XTHREADS 1/' \
		    -e 's/#undef XUSE_MTSAFE_API/#define XUSE_MTSAFE_API 1/' \
		< ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	XlibConf.h

PKGDIST=	X11

MLINKS += AllPlanes.3 BlackPixel.3
MLINKS += AllPlanes.3 ConnectionNumber.3
MLINKS += AllPlanes.3 DefaultColormap.3
MLINKS += AllPlanes.3 DefaultDepth.3
MLINKS += AllPlanes.3 DefaultGC.3
MLINKS += AllPlanes.3 DefaultRootWindow.3
MLINKS += AllPlanes.3 DefaultScreen.3
MLINKS += AllPlanes.3 DefaultScreenOfDisplay.3
MLINKS += AllPlanes.3 DefaultVisual.3
MLINKS += AllPlanes.3 DisplayCells.3
MLINKS += AllPlanes.3 DisplayPlanes.3
MLINKS += AllPlanes.3 DisplayString.3
MLINKS += AllPlanes.3 LastKnownRequestProcessed.3
MLINKS += AllPlanes.3 NextRequest.3
MLINKS += AllPlanes.3 ProtocolRevision.3
MLINKS += AllPlanes.3 ProtocolVersion.3
MLINKS += AllPlanes.3 QLength.3
MLINKS += AllPlanes.3 RootWindow.3
MLINKS += AllPlanes.3 ScreenCount.3
MLINKS += AllPlanes.3 ScreenOfDisplay.3
MLINKS += AllPlanes.3 ServerVendor.3
MLINKS += AllPlanes.3 VendorRelease.3
MLINKS += AllPlanes.3 WhitePixel.3
MLINKS += AllPlanes.3 XExtendedMaxRequestSize.3
MLINKS += AllPlanes.3 XListDepths.3
MLINKS += AllPlanes.3 XMaxRequestSize.3
MLINKS += BlackPixelOfScreen.3 CellsOfScreen.3
MLINKS += BlackPixelOfScreen.3 DefaultColormapOfScreen.3
MLINKS += BlackPixelOfScreen.3 DefaultDepthOfScreen.3
MLINKS += BlackPixelOfScreen.3 DefaultGCOfScreen.3
MLINKS += BlackPixelOfScreen.3 DefaultVisualOfScreen.3
MLINKS += BlackPixelOfScreen.3 DisplayOfScreen.3
MLINKS += BlackPixelOfScreen.3 DoesBackingStore.3
MLINKS += BlackPixelOfScreen.3 DoesSaveUnders.3
MLINKS += BlackPixelOfScreen.3 EventMaskOfScreen.3
MLINKS += BlackPixelOfScreen.3 HeightMMOfScreen.3
MLINKS += BlackPixelOfScreen.3 HeightOfScreen.3
MLINKS += BlackPixelOfScreen.3 MaxCmapsOfScreen.3
MLINKS += BlackPixelOfScreen.3 MinCmapsOfScreen.3
MLINKS += BlackPixelOfScreen.3 PlanesOfScreen.3
MLINKS += BlackPixelOfScreen.3 RootWindowOfScreen.3
MLINKS += BlackPixelOfScreen.3 WhitePixelOfScreen.3
MLINKS += BlackPixelOfScreen.3 WidthMMOfScreen.3
MLINKS += BlackPixelOfScreen.3 WidthOfScreen.3
MLINKS += BlackPixelOfScreen.3 XScreenNumberOfScreen.3
MLINKS += DisplayOfCCC.3 ClientWhitePointOfCCC.3
MLINKS += DisplayOfCCC.3 ScreenNumberOfCCC.3
MLINKS += DisplayOfCCC.3 ScreenWhitePointOfCCC.3
MLINKS += DisplayOfCCC.3 VisualOfCCC.3
MLINKS += ImageByteOrder.3 BitmapBitOrder.3
MLINKS += ImageByteOrder.3 BitmapPad.3
MLINKS += ImageByteOrder.3 BitmapUnit.3
MLINKS += ImageByteOrder.3 DisplayHeight.3
MLINKS += ImageByteOrder.3 DisplayHeightMM.3
MLINKS += ImageByteOrder.3 DisplayWidth.3
MLINKS += ImageByteOrder.3 DisplayWidthMM.3
MLINKS += ImageByteOrder.3 XListPixmapFormats.3
MLINKS += ImageByteOrder.3 XPixmapFormatValues.3
MLINKS += IsCursorKey.3 IsFunctionKey.3
MLINKS += IsCursorKey.3 IsKeypadKey.3
MLINKS += IsCursorKey.3 IsMiscFunctionKey.3
MLINKS += IsCursorKey.3 IsModifierKey.3
MLINKS += IsCursorKey.3 IsPFKey.3
MLINKS += IsCursorKey.3 IsPrivateKeypadKey.3
MLINKS += XAddConnectionWatch.3 XInternalConnectionNumbers.3
MLINKS += XAddConnectionWatch.3 XProcessInternalConnection.3
MLINKS += XAddConnectionWatch.3 XRemoveConnectionWatch.3
MLINKS += XAddHost.3 XAddHosts.3
MLINKS += XAddHost.3 XDisableAccessControl.3
MLINKS += XAddHost.3 XEnableAccessControl.3
MLINKS += XAddHost.3 XHostAddress.3
MLINKS += XAddHost.3 XListHosts.3
MLINKS += XAddHost.3 XRemoveHost.3
MLINKS += XAddHost.3 XRemoveHosts.3
MLINKS += XAddHost.3 XSetAccessControl.3
MLINKS += XAllocClassHint.3 XClassHint.3
MLINKS += XAllocClassHint.3 XGetClassHint.3
MLINKS += XAllocClassHint.3 XSetClassHint.3
MLINKS += XAllocColor.3 XAllocColorCells.3
MLINKS += XAllocColor.3 XAllocColorPlanes.3
MLINKS += XAllocColor.3 XAllocNamedColor.3
MLINKS += XAllocColor.3 XFreeColors.3
MLINKS += XAllocIconSize.3 XGetIconSizes.3
MLINKS += XAllocIconSize.3 XIconSize.3
MLINKS += XAllocIconSize.3 XSetIconSizes.3
MLINKS += XAllocSizeHints.3 XGetWMNormalHints.3
MLINKS += XAllocSizeHints.3 XGetWMSizeHints.3
MLINKS += XAllocSizeHints.3 XSetWMNormalHints.3
MLINKS += XAllocSizeHints.3 XSetWMSizeHints.3
MLINKS += XAllocSizeHints.3 XSizeHints.3
MLINKS += XAllocStandardColormap.3 XGetRGBColormaps.3
MLINKS += XAllocStandardColormap.3 XSetRGBColormaps.3
MLINKS += XAllocStandardColormap.3 XStandardColormap.3
MLINKS += XAllocWMHints.3 XGetWMHints.3
MLINKS += XAllocWMHints.3 XSetWMHints.3
MLINKS += XAllocWMHints.3 XWMHints.3
MLINKS += XAnyEvent.3 XEvent.3
MLINKS += XButtonEvent.3 XKeyEvent.3
MLINKS += XButtonEvent.3 XMotionEvent.3
MLINKS += XChangeKeyboardControl.3 XAutoRepeatOff.3
MLINKS += XChangeKeyboardControl.3 XAutoRepeatOn.3
MLINKS += XChangeKeyboardControl.3 XBell.3
MLINKS += XChangeKeyboardControl.3 XGetKeyboardControl.3
MLINKS += XChangeKeyboardControl.3 XKeyboardControl.3
MLINKS += XChangeKeyboardControl.3 XQueryKeymap.3
MLINKS += XChangeKeyboardMapping.3 XDeleteModifiermapEntry.3
MLINKS += XChangeKeyboardMapping.3 XDisplayKeycodes.3
MLINKS += XChangeKeyboardMapping.3 XFreeModifiermap.3
MLINKS += XChangeKeyboardMapping.3 XGetKeyboardMapping.3
MLINKS += XChangeKeyboardMapping.3 XGetModifierMapping.3
MLINKS += XChangeKeyboardMapping.3 XInsertModifiermapEntry.3
MLINKS += XChangeKeyboardMapping.3 XModifierKeymap.3
MLINKS += XChangeKeyboardMapping.3 XNewModifiermap.3
MLINKS += XChangeKeyboardMapping.3 XSetModifierMapping.3
MLINKS += XChangePointerControl.3 XGetPointerControl.3
MLINKS += XChangeSaveSet.3 XAddToSaveSet.3
MLINKS += XChangeSaveSet.3 XRemoveFromSaveSet.3
MLINKS += XChangeWindowAttributes.3 XSetWindowBackground.3
MLINKS += XChangeWindowAttributes.3 XSetWindowBackgroundPixmap.3
MLINKS += XChangeWindowAttributes.3 XSetWindowBorder.3
MLINKS += XChangeWindowAttributes.3 XSetWindowBorderPixmap.3
MLINKS += XChangeWindowAttributes.3 XSetWindowColormap.3
MLINKS += XClearArea.3 XClearWindow.3
MLINKS += XConfigureWindow.3 XMoveResizeWindow.3
MLINKS += XConfigureWindow.3 XMoveWindow.3
MLINKS += XConfigureWindow.3 XResizeWindow.3
MLINKS += XConfigureWindow.3 XSetWindowBorderWidth.3
MLINKS += XConfigureWindow.3 XWindowChanges.3
MLINKS += XCopyArea.3 XCopyPlane.3
MLINKS += XCreateColormap.3 XColor.3
MLINKS += XCreateColormap.3 XCopyColormapAndFree.3
MLINKS += XCreateColormap.3 XFreeColormap.3
MLINKS += XCreateFontCursor.3 XCreateGlyphCursor.3
MLINKS += XCreateFontCursor.3 XCreatePixmapCursor.3
MLINKS += XCreateFontSet.3 XFreeFontSet.3
MLINKS += XCreateGC.3 XChangeGC.3
MLINKS += XCreateGC.3 XCopyGC.3
MLINKS += XCreateGC.3 XFreeGC.3
MLINKS += XCreateGC.3 XGCValues.3
MLINKS += XCreateGC.3 XGContextFromGC.3
MLINKS += XCreateGC.3 XGetGCValues.3
MLINKS += XCreateIC.3 XDestroyIC.3
MLINKS += XCreateIC.3 XIMOfIC.3
MLINKS += XCreateOC.3 XDestroyOC.3
MLINKS += XCreateOC.3 XGetOCValues.3
MLINKS += XCreateOC.3 XOMOfOC.3
MLINKS += XCreateOC.3 XSetOCValues.3
MLINKS += XCreatePixmap.3 XFreePixmap.3
MLINKS += XCreateRegion.3 XDestroyRegion.3
MLINKS += XCreateRegion.3 XSetRegion.3
MLINKS += XCreateWindow.3 XCreateSimpleWindow.3
MLINKS += XCreateWindow.3 XSetWindowAttributes.3
MLINKS += XDefineCursor.3 XUndefineCursor.3
MLINKS += XDestroyWindow.3 XDestroySubwindows.3
MLINKS += XDrawArc.3 XArc.3
MLINKS += XDrawArc.3 XDrawArcs.3
MLINKS += XDrawImageString.3 XDrawImageString16.3
MLINKS += XDrawLine.3 XDrawLines.3
MLINKS += XDrawLine.3 XDrawSegments.3
MLINKS += XDrawLine.3 XSegment.3
MLINKS += XDrawPoint.3 XDrawPoints.3
MLINKS += XDrawPoint.3 XPoint.3
MLINKS += XDrawRectangle.3 XDrawRectangles.3
MLINKS += XDrawRectangle.3 XRectangle.3
MLINKS += XDrawString.3 XDrawString16.3
MLINKS += XDrawText.3 XDrawText16.3
MLINKS += XDrawText.3 XTextItem.3
MLINKS += XDrawText.3 XTextItem16.3
MLINKS += XEmptyRegion.3 XEqualRegion.3
MLINKS += XEmptyRegion.3 XPointInRegion.3
MLINKS += XEmptyRegion.3 XRectInRegion.3
MLINKS += XFillRectangle.3 XFillArc.3
MLINKS += XFillRectangle.3 XFillArcs.3
MLINKS += XFillRectangle.3 XFillPolygon.3
MLINKS += XFillRectangle.3 XFillRectangles.3
MLINKS += XFlush.3 XEventsQueued.3
MLINKS += XFlush.3 XPending.3
MLINKS += XFlush.3 XSync.3
MLINKS += XFontsOfFontSet.3 XBaseFontNameListOfFontSet.3
MLINKS += XFontsOfFontSet.3 XContextDependentDrawing.3
MLINKS += XFontsOfFontSet.3 XContextualDrawing.3
MLINKS += XFontsOfFontSet.3 XDirectionalDependentDrawing.3
MLINKS += XFontsOfFontSet.3 XLocaleOfFontSet.3
#MLINKS += XGetEventData.3 XFreeEventData.3
#MLINKS += XGetEventData.3 XGenericEventCookie.3
MLINKS += XGetVisualInfo.3 XMatchVisualInfo.3
MLINKS += XGetVisualInfo.3 XVisualIDFromVisual.3
MLINKS += XGetVisualInfo.3 XVisualInfo.3
MLINKS += XGetWindowAttributes.3 XGetGeometry.3
MLINKS += XGetWindowAttributes.3 XWindowAttributes.3
MLINKS += XGetWindowProperty.3 XChangeProperty.3
MLINKS += XGetWindowProperty.3 XDeleteProperty.3
MLINKS += XGetWindowProperty.3 XListProperties.3
MLINKS += XGetWindowProperty.3 XRotateWindowProperties.3
MLINKS += XGrabButton.3 XUngrabButton.3
MLINKS += XGrabKey.3 XUngrabKey.3
MLINKS += XGrabKeyboard.3 XUngrabKeyboard.3
MLINKS += XGrabPointer.3 XChangeActivePointerGrab.3
MLINKS += XGrabPointer.3 XUngrabPointer.3
MLINKS += XGrabServer.3 XUngrabServer.3
MLINKS += XGraphicsExposeEvent.3 XNoExposeEvent.3
MLINKS += XIconifyWindow.3 XReconfigureWMWindow.3
MLINKS += XIconifyWindow.3 XWithdrawWindow.3
MLINKS += XIfEvent.3 XCheckIfEvent.3
MLINKS += XIfEvent.3 XPeekIfEvent.3
MLINKS += XInitImage.3 XAddPixel.3
MLINKS += XInitImage.3 XCreateImage.3
MLINKS += XInitImage.3 XDestroyImage.3
MLINKS += XInitImage.3 XGetPixel.3
MLINKS += XInitImage.3 XPutPixel.3
MLINKS += XInitImage.3 XSubImage.3
MLINKS += XInitThreads.3 XLockDisplay.3
MLINKS += XInitThreads.3 XUnlockDisplay.3
MLINKS += XInstallColormap.3 XListInstalledColormaps.3
MLINKS += XInstallColormap.3 XUninstallColormap.3
MLINKS += XInternAtom.3 XGetAtomName.3
MLINKS += XInternAtom.3 XGetAtomNames.3
MLINKS += XInternAtom.3 XInternAtoms.3
MLINKS += XIntersectRegion.3 XOffsetRegion.3
MLINKS += XIntersectRegion.3 XShrinkRegion.3
MLINKS += XIntersectRegion.3 XSubtractRegion.3
MLINKS += XIntersectRegion.3 XUnionRectWithRegion.3
MLINKS += XIntersectRegion.3 XUnionRegion.3
MLINKS += XIntersectRegion.3 XXorRegion.3
MLINKS += XListFonts.3 XFreeFontInfo.3
MLINKS += XListFonts.3 XFreeFontNames.3
MLINKS += XListFonts.3 XListFontsWithInfo.3
MLINKS += XLoadFont.3 XChar2b.3
MLINKS += XLoadFont.3 XCharStruct.3
MLINKS += XLoadFont.3 XFontProp.3
MLINKS += XLoadFont.3 XFontStruct.3
MLINKS += XLoadFont.3 XFreeFont.3
MLINKS += XLoadFont.3 XGetFontProperty.3
MLINKS += XLoadFont.3 XLoadQueryFont.3
MLINKS += XLoadFont.3 XQueryFont.3
MLINKS += XLoadFont.3 XUnloadFont.3
MLINKS += XLookupKeysym.3 XLookupString.3
MLINKS += XLookupKeysym.3 XRebindKeysym.3
MLINKS += XLookupKeysym.3 XRefreshKeyboardMapping.3
MLINKS += XMapEvent.3 XMappingEvent.3
MLINKS += XMapWindow.3 XMapRaised.3
MLINKS += XMapWindow.3 XMapSubwindows.3
MLINKS += XNextEvent.3 XCheckMaskEvent.3
MLINKS += XNextEvent.3 XCheckTypedEvent.3
MLINKS += XNextEvent.3 XCheckTypedWindowEvent.3
MLINKS += XNextEvent.3 XCheckWindowEvent.3
MLINKS += XNextEvent.3 XMaskEvent.3
MLINKS += XNextEvent.3 XPeekEvent.3
MLINKS += XNextEvent.3 XWindowEvent.3
MLINKS += XOpenDisplay.3 XCloseDisplay.3
MLINKS += XOpenIM.3 XCloseIM.3
MLINKS += XOpenIM.3 XDisplayOfIM.3
MLINKS += XOpenIM.3 XGetIMValues.3
MLINKS += XOpenIM.3 XLocaleOfIM.3
MLINKS += XOpenIM.3 XRegisterIMInstantiateCallback.3
MLINKS += XOpenIM.3 XSetIMValues.3
MLINKS += XOpenIM.3 XUnregisterIMInstantiateCallback.3
MLINKS += XOpenOM.3 XCloseOM.3
MLINKS += XOpenOM.3 XDisplayOfOM.3
MLINKS += XOpenOM.3 XGetOMValues.3
MLINKS += XOpenOM.3 XLocaleOfOM.3
MLINKS += XOpenOM.3 XSetOMValues.3
MLINKS += XParseGeometry.3 XWMGeometry.3
MLINKS += XPolygonRegion.3 XClipBox.3
MLINKS += XPutImage.3 XGetImage.3
MLINKS += XPutImage.3 XGetSubImage.3
MLINKS += XQueryBestSize.3 XQueryBestStipple.3
MLINKS += XQueryBestSize.3 XQueryBestTile.3
MLINKS += XQueryColor.3 XLookupColor.3
MLINKS += XQueryColor.3 XParseColor.3
MLINKS += XQueryColor.3 XQueryColors.3
MLINKS += XQueryExtension.3 XFreeExtensionList.3
MLINKS += XQueryExtension.3 XListExtensions.3
MLINKS += XRaiseWindow.3 XCirculateSubwindows.3
MLINKS += XRaiseWindow.3 XCirculateSubwindowsDown.3
MLINKS += XRaiseWindow.3 XCirculateSubwindowsUp.3
MLINKS += XRaiseWindow.3 XLowerWindow.3
MLINKS += XRaiseWindow.3 XRestackWindows.3
MLINKS += XReadBitmapFile.3 XCreateBitmapFromData.3
MLINKS += XReadBitmapFile.3 XCreatePixmapFromBitmapData.3
MLINKS += XReadBitmapFile.3 XReadBitmapFileData.3
MLINKS += XReadBitmapFile.3 XWriteBitmapFile.3
MLINKS += XRecolorCursor.3 XFreeCursor.3
MLINKS += XRecolorCursor.3 XQueryBestCursor.3
MLINKS += XResourceManagerString.3 XScreenResourceString.3
MLINKS += XSaveContext.3 XDeleteContext.3
MLINKS += XSaveContext.3 XFindContext.3
MLINKS += XSaveContext.3 XUniqueContext.3
MLINKS += XSendEvent.3 XDisplayMotionBufferSize.3
MLINKS += XSendEvent.3 XGetMotionEvents.3
MLINKS += XSendEvent.3 XTimeCoord.3
MLINKS += XSetArcMode.3 XSetGraphicsExposure.3
MLINKS += XSetArcMode.3 XSetSubwindowMode.3
MLINKS += XSetClipOrigin.3 XSetClipMask.3
MLINKS += XSetClipOrigin.3 XSetClipRectangles.3
MLINKS += XSetCloseDownMode.3 XKillClient.3
MLINKS += XSetCommand.3 XGetCommand.3
MLINKS += XSetErrorHandler.3 XDisplayName.3
MLINKS += XSetErrorHandler.3 XGetErrorDatabaseText.3
MLINKS += XSetErrorHandler.3 XGetErrorText.3
MLINKS += XSetErrorHandler.3 XSetIOErrorHandler.3
MLINKS += XSetFillStyle.3 XSetFillRule.3
MLINKS += XSetFontPath.3 XFreeFontPath.3
MLINKS += XSetFontPath.3 XGetFontPath.3
MLINKS += XSetICFocus.3 XUnsetICFocus.3
MLINKS += XSetICValues.3 XGetICValues.3
MLINKS += XSetInputFocus.3 XGetInputFocus.3
MLINKS += XSetLineAttributes.3 XSetDashes.3
MLINKS += XSetPointerMapping.3 XGetPointerMapping.3
MLINKS += XSetScreenSaver.3 XActivateScreenSaver.3
MLINKS += XSetScreenSaver.3 XForceScreenSaver.3
MLINKS += XSetScreenSaver.3 XGetScreenSaver.3
MLINKS += XSetScreenSaver.3 XResetScreenSaver.3
MLINKS += XSetSelectionOwner.3 XConvertSelection.3
MLINKS += XSetSelectionOwner.3 XGetSelectionOwner.3
MLINKS += XSetState.3 XSetBackground.3
MLINKS += XSetState.3 XSetForeground.3
MLINKS += XSetState.3 XSetFunction.3
MLINKS += XSetState.3 XSetPlaneMask.3
MLINKS += XSetTextProperty.3 XGetTextProperty.3
MLINKS += XSetTile.3 XSetStipple.3
MLINKS += XSetTile.3 XSetTSOrigin.3
MLINKS += XSetTransientForHint.3 XGetTransientForHint.3
MLINKS += XSetWMClientMachine.3 XGetWMClientMachine.3
MLINKS += XSetWMColormapWindows.3 XGetWMColormapWindows.3
MLINKS += XSetWMIconName.3 XGetIconName.3
MLINKS += XSetWMIconName.3 XGetWMIconName.3
MLINKS += XSetWMIconName.3 XSetIconName.3
MLINKS += XSetWMName.3 XFetchName.3
MLINKS += XSetWMName.3 XGetWMName.3
MLINKS += XSetWMName.3 XStoreName.3
MLINKS += XSetWMProperties.3 XmbSetWMProperties.3
MLINKS += XSetWMProperties.3 Xutf8SetWMProperties.3
MLINKS += XSetWMProtocols.3 XGetWMProtocols.3
MLINKS += XStoreBytes.3 XFetchBuffer.3
MLINKS += XStoreBytes.3 XFetchBytes.3
MLINKS += XStoreBytes.3 XRotateBuffers.3
MLINKS += XStoreBytes.3 XStoreBuffer.3
MLINKS += XStoreColors.3 XStoreColor.3
MLINKS += XStoreColors.3 XStoreNamedColor.3
MLINKS += XStringListToTextProperty.3 XFreeStringList.3
MLINKS += XStringListToTextProperty.3 XTextProperty.3
MLINKS += XStringListToTextProperty.3 XTextPropertyToStringList.3
MLINKS += XStringToKeysym.3 XConvertCase.3
MLINKS += XStringToKeysym.3 XKeycodeToKeysym.3
MLINKS += XStringToKeysym.3 XKeysymToKeycode.3
MLINKS += XStringToKeysym.3 XKeysymToString.3
MLINKS += XSupportsLocale.3 XSetLocaleModifiers.3
MLINKS += XSynchronize.3 XSetAfterFunction.3
MLINKS += XTextExtents.3 XQueryTextExtents.3
MLINKS += XTextExtents.3 XQueryTextExtents16.3
MLINKS += XTextExtents.3 XTextExtents16.3
MLINKS += XTextWidth.3 XTextWidth16.3
MLINKS += XUnmapWindow.3 XUnmapSubwindows.3
MLINKS += XcmsAllocColor.3 XcmsAllocNamedColor.3
MLINKS += XcmsCCCOfColormap.3 XcmsSetCCCOfColormap.3
MLINKS += XcmsCIELabQueryMaxC.3 XcmsCIELabQueryMaxL.3
MLINKS += XcmsCIELabQueryMaxC.3 XcmsCIELabQueryMaxLC.3
MLINKS += XcmsCIELabQueryMaxC.3 XcmsCIELabQueryMinL.3
MLINKS += XcmsCIELuvQueryMaxC.3 XcmsCIELuvQueryMaxL.3
MLINKS += XcmsCIELuvQueryMaxC.3 XcmsCIELuvQueryMaxLC.3
MLINKS += XcmsCIELuvQueryMaxC.3 XcmsCIELuvQueryMinL.3
MLINKS += XcmsColor.3 XcmsCIELab.3
MLINKS += XcmsColor.3 XcmsCIELuv.3
MLINKS += XcmsColor.3 XcmsCIEXYZ.3
MLINKS += XcmsColor.3 XcmsCIEuvY.3
MLINKS += XcmsColor.3 XcmsCIExyY.3
MLINKS += XcmsColor.3 XcmsPad.3
MLINKS += XcmsColor.3 XcmsRGB.3
MLINKS += XcmsColor.3 XcmsRGBi.3
MLINKS += XcmsColor.3 XcmsTekHVC.3
MLINKS += XcmsCreateCCC.3 XcmsFreeCCC.3
MLINKS += XcmsQueryBlack.3 XcmsQueryBlue.3
MLINKS += XcmsQueryBlack.3 XcmsQueryGreen.3
MLINKS += XcmsQueryBlack.3 XcmsQueryRed.3
MLINKS += XcmsQueryBlack.3 XcmsQueryWhite.3
MLINKS += XcmsQueryColor.3 XcmsLookupColor.3
MLINKS += XcmsQueryColor.3 XcmsQueryColors.3
MLINKS += XcmsSetWhitePoint.3 XcmsSetWhiteAdjustProc.3
MLINKS += XcmsStoreColor.3 XcmsStoreColors.3
MLINKS += XcmsTekHVCQueryMaxC.3 XcmsTekHVCQueryMaxV.3
MLINKS += XcmsTekHVCQueryMaxC.3 XcmsTekHVCQueryMaxVC.3
MLINKS += XcmsTekHVCQueryMaxC.3 XcmsTekHVCQueryMaxVSamples.3
MLINKS += XcmsTekHVCQueryMaxC.3 XcmsTekHVCQueryMinV.3
MLINKS += XmbDrawImageString.3 Xutf8DrawImageString.3
MLINKS += XmbDrawImageString.3 XwcDrawImageString.3
MLINKS += XmbDrawString.3 Xutf8DrawString.3
MLINKS += XmbDrawString.3 XwcDrawString.3
MLINKS += XmbDrawText.3 Xutf8DrawText.3
MLINKS += XmbDrawText.3 XwcDrawText.3
MLINKS += XmbLookupString.3 Xutf8LookupString.3
MLINKS += XmbLookupString.3 XwcLookupString.3
MLINKS += XmbResetIC.3 Xutf8ResetIC.3
MLINKS += XmbResetIC.3 XwcResetIC.3
MLINKS += XmbTextEscapement.3 Xutf8TextEscapement.3
MLINKS += XmbTextEscapement.3 XwcTextEscapement.3
MLINKS += XmbTextExtents.3 Xutf8TextExtents.3
MLINKS += XmbTextExtents.3 XwcTextExtents.3
MLINKS += XmbTextListToTextProperty.3 XDefaultString.3
MLINKS += XmbTextListToTextProperty.3 XmbTextPropertyToTextList.3
MLINKS += XmbTextListToTextProperty.3 Xutf8TextListToTextProperty.3
MLINKS += XmbTextListToTextProperty.3 Xutf8TextPropertyToTextList.3
MLINKS += XmbTextListToTextProperty.3 XwcFreeStringList.3
MLINKS += XmbTextListToTextProperty.3 XwcTextListToTextProperty.3
MLINKS += XmbTextListToTextProperty.3 XwcTextPropertyToTextList.3
MLINKS += XmbTextPerCharExtents.3 Xutf8TextPerCharExtents.3
MLINKS += XmbTextPerCharExtents.3 XwcTextPerCharExtents.3
MLINKS += XrmGetFileDatabase.3 XrmDestroyDatabase.3
MLINKS += XrmGetFileDatabase.3 XrmGetDatabase.3
MLINKS += XrmGetFileDatabase.3 XrmGetStringDatabase.3
MLINKS += XrmGetFileDatabase.3 XrmLocaleOfDatabase.3
MLINKS += XrmGetFileDatabase.3 XrmPutFileDatabase.3
MLINKS += XrmGetFileDatabase.3 XrmSetDatabase.3
MLINKS += XrmGetResource.3 XrmQGetResource.3
MLINKS += XrmGetResource.3 XrmQGetSearchList.3
MLINKS += XrmGetResource.3 XrmQGetSearchResource.3
MLINKS += XrmInitialize.3 XrmOptionDescRec.3
MLINKS += XrmInitialize.3 XrmOptionKind.3
MLINKS += XrmInitialize.3 XrmParseCommand.3
MLINKS += XrmInitialize.3 XrmValue.3
MLINKS += XrmMergeDatabases.3 XrmCombineDatabase.3
MLINKS += XrmMergeDatabases.3 XrmCombineFileDatabase.3
MLINKS += XrmPutResource.3 XrmPutLineResource.3
MLINKS += XrmPutResource.3 XrmPutStringResource.3
MLINKS += XrmPutResource.3 XrmQPutResource.3
MLINKS += XrmPutResource.3 XrmQPutStringResource.3
MLINKS += XrmUniqueQuark.3 XrmPermStringToQuark.3
MLINKS += XrmUniqueQuark.3 XrmQuarkToString.3
MLINKS += XrmUniqueQuark.3 XrmStringToBindingQuarkList.3
MLINKS += XrmUniqueQuark.3 XrmStringToQuark.3
MLINKS += XrmUniqueQuark.3 XrmStringToQuarkList.3

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
