#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 19:26:51 lukem Exp $

.include <bsd.own.mk>

MAN=	XtAppAddActionHook.3 XtAddActions.3 XtAddCallback.3 \
	XtAddEventHandler.3 XtAddExposureToRegion.3 XtAddGrab.3 \
	XtAddInput.3 XtAllocateGC.3 XtAppAddActions.3 XtAppAddConverter.3 \
	XtAppAddInput.3 XtAppAddSignal.3 XtAppAddTimeOut.3 \
	XtAppAddWorkProc.3 XtAppCreateShell.3 XtAppError.3 \
	XtAppErrorMsg.3 XtAppGetErrorDatabase.3 XtAppGetSelectionTimeout.3 \
	XtAppInitialize.3 XtAppLock.3 XtAppNextEvent.3 \
	XtAppReleaseCacheRefs.3 XtAppSetExitFlag.3 XtAppSetTypeConverter.3 \
	XtAppSetFallbackResources.3 XtBuildEventMask.3 XtAppAddBlockHook.3 \
	XtCallAcceptFocus.3 XtCallActionProc.3 XtCallCallbacks.3 \
	XtClass.3 XtSetMultiClickTime.3 XtConfigureWidget.3 \
	XtConvertAndStore.3 XtConvert.3 XtCreateApplicationContext.3 \
	XtCreateApplicationShell.3 XtCreatePopupShell.3 XtCreateWidget.3 \
	XtCreateWindow.3 XtCreateSelectionRequest.3 \
	XtDisplayStringConversionWarning.3 XtDisplayToApplicationContext.3 \
	XtDisplay.3 XtDisplayInitialize.3 XtError.3 XtErrorMsg.3 \
	XtInsertEventTypeHandler.3 XtFindFile.3 XtGetErrorDatabase.3 \
	XtGetApplicationNameAndClass.3 XtGetActionKeysym.3 \
	XtGetActionList.3 XtGetApplicationResources.3 XtGetClassExtension.3 \
	XtGetDisplays.3 XtGetGC.3 XtGetKeyboardFocusWidget.3 \
	XtGetResourceList.3 XtGetSelectionParameters.3 \
	XtGetSelectionRequest.3 XtGetSelectionTimeout.3 \
	XtGetSelectionValue.3 XtGetSelectionValueIncremental.3 \
	XtGetSubresources.3 XtGrabKey.3 XtHooksOfDisplay.3 \
	XtInitialize.3 XtInitializeWidgetClass.3 XtGetKeysymTable.3 \
	XtLastEventProcessed.3 XtMakeGeometryRequest.3 XtMalloc.3 \
	XtManageChildren.3 XtMapWidget.3 XtName.3 XtNameToWidget.3 \
	XtNextEvent.3 XtOffset.3 XtOpenApplication.3 XtOwnSelection.3 \
	XtParseAcceleratorTable.3 XtParseTranslationTable.3 XtParent.3 \
	XtPopdown.3 XtPopup.3 XtProcessLock.3 XtQueryGeometry.3 \
	XtRealizeWidget.3 XtRegisterDrawable.3 XtRegisterGrabAction.3 \
	XtReservePropertyAtom.3 XtResolvePathname.3 XtSessionGetToken.3 \
	XtSetArg.3 XtSetKeyboardFocus.3 XtSetKeyTranslator.3 \
	XtSetLanguageProc.3 XtSetSelectionParameters.3 XtSetSensitive.3 \
	XtSetValues.3 XtSetWMColormapWindows.3 XtStringConversionWarning.3 \
	XtToolkitThreadInitialize.3 XtTranslateCoords.3 \
	XtVaCreateArgsList.3

MLINKS=	XtAddCallback.3 XtAddCallbacks.3 \
	XtAddCallback.3 XtRemoveCallback.3 \
	XtAddCallback.3 XtRemoveCallbacks.3 \
	XtAddCallback.3 XtRemoveAllCallbacks.3 \
	XtAddEventHandler.3 XtAddRawEventHandler.3 \
	XtAddEventHandler.3 XtRemoveEventHandler.3 \
	XtAddEventHandler.3 XtRemoveRawEventHandler.3 \
	XtAddEventHandler.3 XtInsertEventHandler.3 \
	XtAddEventHandler.3 XtInsertRawEventHandler.3 \
	XtAddGrab.3 XtRemoveGrab.3 \
	XtAddInput.3 XtAddTimeOut.3 \
	XtAddInput.3 XtAddTimeout.3 \
	XtAppAddActionHook.3 XtRemoveActionHook.3 \
	XtAppAddBlockHook.3 XtRemoveBlockHook.3 \
	XtAppAddConverter.3 XtAddConverter.3 \
	XtAppAddInput.3 XtRemoveInput.3 \
	XtAppAddSignal.3 XtNoticeSignal.3 \
	XtAppAddSignal.3 XtRemoveSignal.3 \
	XtAppAddTimeOut.3 XtRemoveTimeOut.3 \
	XtAppAddTimeOut.3 XtAppAddTimeout.3 \
	XtAppAddWorkProc.3 XtRemoveWorkProc.3 \
	XtAppCreateShell.3 XtVaAppCreateShell.3 \
	XtAppError.3 XtAppSetErrorHandler.3 \
	XtAppError.3 XtAppSetWarningHandler.3 \
	XtAppError.3 XtAppWarning.3 \
	XtAppErrorMsg.3 XtAppSetErrorMsgHandler.3 \
	XtAppErrorMsg.3 XtAppSetWarningMsgHandler.3 \
	XtAppErrorMsg.3 XtAppWarningMsg.3 \
	XtAppGetErrorDatabase.3 XtAppGetErrorDatabaseText.3 \
	XtAppGetSelectionTimeout.3 XtAppSetSelectionTimeout.3 \
	XtAppInitialize.3 XtVaAppInitialize.3 \
	XtAppLock.3 XtAppUnlock.3 \
	XtAppNextEvent.3 XtAppPending.3 \
	XtAppNextEvent.3 XtAppPeekEvent.3 \
	XtAppNextEvent.3 XtAppProcessEvent.3 \
	XtAppNextEvent.3 XtDispatchEvent.3 \
	XtAppNextEvent.3 XtAppMainLoop.3 \
	XtAppSetExitFlag.3 XtAppGetExitFlag.3 \
	XtAppSetTypeConverter.3 XtSetTypeConverter.3 \
	XtCallCallbacks.3 XtCallCallbackList.3 \
	XtCallCallbacks.3 XtHasCallbacks.3 \
	XtClass.3 XtSuperclass.3 \
	XtClass.3 XtIsSubclass.3 \
	XtClass.3 XtCheckSubclass.3 \
	XtClass.3 XtIsObject.3 \
	XtClass.3 XtIsRectObj.3 \
	XtClass.3 XtIsWidget.3 \
	XtClass.3 XtIsComposite.3 \
	XtClass.3 XtIsConstraint.3 \
	XtClass.3 XtIsShell.3 \
	XtClass.3 XtIsOverrideShell.3 \
	XtClass.3 XtIsWMShell.3 \
	XtClass.3 XtIsVendorShell.3 \
	XtClass.3 XtIsTransientShell.3 \
	XtClass.3 XtIsTopLevelShell.3 \
	XtClass.3 XtIsApplicationShell.3 \
	XtClass.3 XtIsSessionShell.3 \
	XtConfigureWidget.3 XtMoveWidget.3 \
	XtConfigureWidget.3 XtResizeWidget.3 \
	XtConvert.3 XtDirectConvert.3 \
	XtConvertAndStore.3 XtCallConverter.3 \
	XtCreateApplicationContext.3 XtDestroyApplicationContext.3 \
	XtCreateApplicationContext.3 XtWidgetToApplicationContext.3 \
	XtCreateApplicationContext.3 XtToolkitInitialize.3 \
	XtCreatePopupShell.3 XtVaCreatePopupShell.3 \
	XtCreateSelectionRequest.3 XtSendSelectionRequest.3 \
	XtCreateSelectionRequest.3 XtCancelSelectionRequest.3 \
	XtCreateWidget.3 XtVaCreateWidget.3 \
	XtCreateWidget.3 XtCreateManagedWidget.3 \
	XtCreateWidget.3 XtVaCreateManagedWidget.3 \
	XtCreateWidget.3 XtDestroyWidget.3 \
	XtDisplay.3 XtDisplayOfObject.3 \
	XtDisplay.3 XtScreen.3 \
	XtDisplay.3 XtScreenOfObject.3 \
	XtDisplay.3 XtWindow.3 \
	XtDisplay.3 XtWindowOfObject.3 \
	XtDisplayInitialize.3 XtOpenDisplay.3 \
	XtDisplayInitialize.3 XtCloseDisplay.3 \
	XtDisplayInitialize.3 XtDatabase.3 \
	XtDisplayInitialize.3 XtScreenDatabase.3 \
	XtError.3 XtSetErrorHandler.3 \
	XtError.3 XtSetWarningHandler.3 \
	XtError.3 XtWarning.3 \
	XtErrorMsg.3 XtSetErrorMsgHandler.3 \
	XtErrorMsg.3 XtSetWarningMsgHandler.3 \
	XtErrorMsg.3 XtWarningMsg.3 \
	XtGetApplicationResources.3 XtVaGetApplicationResources.3 \
	XtGetErrorDatabase.3 XtGetErrorDatabaseText.3 \
	XtGetGC.3 XtReleaseGC.3 \
	XtGetKeysymTable.3 XtKeysymToKeycodeList.3 \
	XtGetResourceList.3 XtGetConstraintResourceList.3 \
	XtGetSelectionTimeout.3 XtSetSelectionTimeout.3 \
	XtGetSelectionValue.3 XtGetSelectionValues.3 \
	XtGetSelectionValueIncremental.3 XtGetSelectionValuesIncremental.3 \
	XtGetSubresources.3 XtVaGetSubresources.3 \
	XtGrabKey.3 XtUngrabKey.3 \
	XtGrabKey.3 XtGrabKeyboard.3 \
	XtGrabKey.3 XtUngrabKeyboard.3 \
	XtGrabKey.3 XtGrabButton.3 \
	XtGrabKey.3 XtUngrabButton.3 \
	XtGrabKey.3 XtGrabPointer.3 \
	XtGrabKey.3 XtUngrabPointer.3 \
	XtInsertEventTypeHandler.3 XtRemoveEventTypeHandler.3 \
	XtInsertEventTypeHandler.3 XtRegisterExtensionSelector.3 \
	XtInsertEventTypeHandler.3 XtSetEventDispatcher.3 \
	XtInsertEventTypeHandler.3 XtDispatchEventToWidget.3 \
	XtLastEventProcessed.3 XtLastTimestampProcessed.3 \
	XtMakeGeometryRequest.3 XtMakeResizeRequest.3 \
	XtMalloc.3 XtCalloc.3 \
	XtMalloc.3 XtRealloc.3 \
	XtMalloc.3 XtFree.3 \
	XtMalloc.3 XtNew.3 \
	XtMalloc.3 XtNewString.3 \
	XtManageChildren.3 XtManageChild.3 \
	XtManageChildren.3 XtUnmanageChildren.3 \
	XtManageChildren.3 XtUnmanageChild.3 \
	XtManageChildren.3 XtIsManaged.3 \
	XtManageChildren.3 XtChangeManagedSet.3 \
	XtMapWidget.3 XtSetMappedWhenManaged.3 \
	XtMapWidget.3 XtUnmapWidget.3 \
	XtNameToWidget.3 XtWindowToWidget.3 \
	XtNextEvent.3 XtPending.3 \
	XtNextEvent.3 XtPeekEvent.3 \
	XtNextEvent.3 XtProcessEvent.3 \
	XtNextEvent.3 XtMainLoop.3 \
	XtOffset.3 XtOffsetOf.3 \
	XtOffset.3 XtNumber.3 \
	XtOpenApplication.3 XtVaOpenApplication.3 \
	XtOwnSelection.3 XtOwnSelectionIncremental.3 \
	XtOwnSelection.3 XtDisownSelection.3 \
	XtParseAcceleratorTable.3 XtInstallAccelerators.3 \
	XtParseAcceleratorTable.3 XtInstallAllAccelerators.3 \
	XtParseTranslationTable.3 XtAugmentTranslations.3 \
	XtParseTranslationTable.3 XtOverrideTranslations.3 \
	XtParseTranslationTable.3 XtUninstallTranslations.3 \
	XtPopdown.3 XtCallbackPopdown.3 \
	XtPopdown.3 MenuPopdown.3 \
	XtPopup.3 XtPopupSpringLoaded.3 \
	XtPopup.3 XtCallbackNone.3 \
	XtPopup.3 XtCallbackNonexclusive.3 \
	XtPopup.3 XtCallbackExclusive.3 \
	XtPopup.3 MenuPopup.3 \
	XtProcessLock.3 XtProcessUnlock.3 \
	XtRealizeWidget.3 XtIsRealized.3 \
	XtRealizeWidget.3 XtUnrealizeWidget.3 \
	XtRegisterDrawable.3 XtUnregisterDrawable.3 \
	XtReservePropertyAtom.3 XtReleasePropertyAtom.3 \
	XtSessionGetToken.3 XtSessionReturnToken.3 \
	XtSetArg.3 XtMergeArgLists.3 \
	XtSetKeyTranslator.3 XtTranslateKeycode.3 \
	XtSetKeyTranslator.3 XtRegisterCaseConverter.3 \
	XtSetKeyTranslator.3 XtConvertCase.3 \
	XtSetMultiClickTime.3 XtGetMultiClickTime.3 \
	XtSetSensitive.3 XtIsSensitive.3 \
	XtSetValues.3 XtVaSetValues.3 \
	XtSetValues.3 XtSetSubvalues.3 \
	XtSetValues.3 XtGetValues.3 \
	XtSetValues.3 XtVaGetValues.3 \
	XtSetValues.3 XtGetSubvalues.3 \
	XtSetValues.3 XtVaGetSubvalues.3 \
	XtSetValues.3 XtVaSetSubvalues.3

LNFILES=XtActHook.man XtAppAddActionHook.man \
	XtAddAct.man XtAddActions.man \
	XtAddCbk.man XtAddCallback.man \
	XtAddEHand.man XtAddEventHandler.man \
	XtAddETReg.man XtAddExposureToRegion.man \
	XtAddIn.man XtAddInput.man \
	XtAllocGC.man XtAllocateGC.man \
	XtAppAAct.man XtAppAddActions.man \
	XtAppAC.man XtAppAddConverter.man \
	XtAppAI.man XtAppAddInput.man \
	XtAppASig.man XtAppAddSignal.man \
	XtAppATO.man XtAppAddTimeOut.man \
	XtAppAWP.man XtAppAddWorkProc.man \
	XtAppCSh.man XtAppCreateShell.man \
	XtAppE.man XtAppError.man \
	XtAppEM.man XtAppErrorMsg.man \
	XtAppGEDB.man XtAppGetErrorDatabase.man \
	XtAppGSTO.man XtAppGetSelectionTimeout.man \
	XtAppInit.man XtAppInitialize.man \
	XtAppNEv.man XtAppNextEvent.man \
	XtAppRCR.man XtAppReleaseCacheRefs.man \
	XtAppSEF.man XtAppSetExitFlag.man \
	XtAppSTC.man XtAppSetTypeConverter.man \
	XtAppSetFR.man XtAppSetFallbackResources.man \
	XtBEMask.man XtBuildEventMask.man \
	XtBlockH.man XtAppAddBlockHook.man \
	XtCallAFoc.man XtCallAcceptFocus.man \
	XtCallActP.man XtCallActionProc.man \
	XtCallCbks.man XtCallCallbacks.man \
	XtClickT.man XtSetMultiClickTime.man \
	XtConfWid.man XtConfigureWidget.man \
	XtConvSt.man XtConvertAndStore.man \
	XtCreACon.man XtCreateApplicationContext.man \
	XtCreASh.man XtCreateApplicationShell.man \
	XtCrePSh.man XtCreatePopupShell.man \
	XtCreWid.man XtCreateWidget.man \
	XtCreWin.man XtCreateWindow.man \
	XtCreateSR.man XtCreateSelectionRequest.man \
	XtDStrCW.man XtDisplayStringConversionWarning.man \
	XtDisACon.man XtDisplayToApplicationContext.man \
	XtDisplayI.man XtDisplayInitialize.man \
	XtErr.man XtError.man \
	XtErrM.man XtErrorMsg.man \
	XtExtEvDis.man XtInsertEventTypeHandler.man \
	XtFindF.man XtFindFile.man \
	XtGEDB.man XtGetErrorDatabase.man \
	XtGetANC.man XtGetApplicationNameAndClass.man \
	XtGetActK.man XtGetActionKeysym.man \
	XtGetActL.man XtGetActionList.man \
	XtGetAres.man XtGetApplicationResources.man \
	XtGetClExt.man XtGetClassExtension.man \
	XtGetDisp.man XtGetDisplays.man \
	XtGetKFoc.man XtGetKeyboardFocusWidget.man \
	XtGetRList.man XtGetResourceList.man \
	XtGetSP.man XtGetSelectionParameters.man \
	XtGetSR.man XtGetSelectionRequest.man \
	XtGetSTO.man XtGetSelectionTimeout.man \
	XtGetSVal.man XtGetSelectionValue.man \
	XtGetSValI.man XtGetSelectionValueIncremental.man \
	XtGetSres.man XtGetSubresources.man \
	XtHookOD.man XtHooksOfDisplay.man \
	XtInit.man XtInitialize.man \
	XtInitWC.man XtInitializeWidgetClass.man \
	XtKeysym.man XtGetKeysymTable.man \
	XtLastProc.man XtLastEventProcessed.man \
	XtMakGReq.man XtMakeGeometryRequest.man \
	XtManChild.man XtManageChildren.man \
	XtMapWid.man XtMapWidget.man \
	XtNameTWid.man XtNameToWidget.man \
	XtNextEv.man XtNextEvent.man \
	XtOpenApp.man XtOpenApplication.man \
	XtOwnSel.man XtOwnSelection.man \
	XtParATab.man XtParseAcceleratorTable.man \
	XtParTTab.man XtParseTranslationTable.man \
	XtProcLock.man XtProcessLock.man \
	XtQueryGeo.man XtQueryGeometry.man \
	XtRealize.man XtRealizeWidget.man \
	XtRegDraw.man XtRegisterDrawable.man \
	XtRegGA.man XtRegisterGrabAction.man \
	XtResPA.man XtReservePropertyAtom.man \
	XtResPath.man XtResolvePathname.man \
	XtSession.man XtSessionGetToken.man \
	XtSetKFoc.man XtSetKeyboardFocus.man \
	XtSetKTr.man XtSetKeyTranslator.man \
	XtSetLangP.man XtSetLanguageProc.man \
	XtSetSP.man XtSetSelectionParameters.man \
	XtSetSens.man XtSetSensitive.man \
	XtSetVal.man XtSetValues.man \
	XtSetWMC.man XtSetWMColormapWindows.man \
	XtStrCW.man XtStringConversionWarning.man \
	XtThreadI.man XtToolkitThreadInitialize.man \
	XtTransC.man XtTranslateCoords.man \
	XtVaCrArgL.man XtVaCreateArgsList.man

.PATH:  ${X11SRCDIR.xc}/doc/man/Xt

.include <bsd.man.mk>
.include <bsd.x11.mk>
