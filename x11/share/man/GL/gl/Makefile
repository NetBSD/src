#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:54 lukem Exp $

.include <bsd.own.mk>

MAN=	glAccum.3 glActiveTextureARB.3 glAlphaFunc.3 \
	glAreTexturesResident.3 glArrayElement.3 glBegin.3 \
	glBindTexture.3 glBitmap.3 glBlendColor.3 glBlendEquation.3 \
	glBlendFunc.3 glCallList.3 glCallLists.3 glClear.3 \
	glClearAccum.3 glClearColor.3 glClearDepth.3 glClearIndex.3 \
	glClearStencil.3 glClientActiveTextureARB.3 glClipPlane.3 \
	glColor.3 glColorMask.3 glColorMaterial.3 glColorPointer.3 \
	glColorSubTable.3 glColorTable.3 glColorTableParameter.3 \
	glConvolutionFilter1D.3 glConvolutionFilter2D.3 \
	glConvolutionParameter.3 glCopyColorSubTable.3 glCopyColorTable.3 \
	glCopyConvolutionFilter1D.3 glCopyConvolutionFilter2D.3 \
	glCopyPixels.3 glCopyTexImage1D.3 glCopyTexImage2D.3 \
	glCopyTexSubImage1D.3 glCopyTexSubImage2D.3 glCopyTexSubImage3D.3 \
	glCullFace.3 glDeleteLists.3 glDeleteTextures.3 glDepthFunc.3 \
	glDepthMask.3 glDepthRange.3 glDrawArrays.3 glDrawBuffer.3 \
	glDrawElements.3 glDrawPixels.3 glDrawRangeElements.3 \
	glEdgeFlag.3 glEdgeFlagPointer.3 glEnable.3 glEnableClientState.3 \
	glEvalCoord.3 glEvalMesh.3 glEvalPoint.3 glFeedbackBuffer.3 \
	glFinish.3 glFlush.3 glFog.3 glFrontFace.3 glFrustum.3 \
	glGenLists.3 glGenTextures.3 glGet.3 glGetClipPlane.3 \
	glGetColorTable.3 glGetColorTableParameter.3 \
	glGetConvolutionFilter.3 glGetConvolutionParameter.3 \
	glGetError.3 glGetHistogram.3 glGetHistogramParameter.3 \
	glGetLight.3 glGetMap.3 glGetMaterial.3 glGetMinmax.3 \
	glGetMinmaxParameter.3 glGetPixelMap.3 glGetPointerv.3 \
	glGetPolygonStipple.3 glGetSeparableFilter.3 glGetString.3 \
	glGetTexEnv.3 glGetTexGen.3 glGetTexImage.3 \
	glGetTexLevelParameter.3 glGetTexParameter.3 glHint.3 \
	glHistogram.3 glIndex.3 glIndexMask.3 glIndexPointer.3 \
	glInitNames.3 glInterleavedArrays.3 glIsEnabled.3 glIsTexture.3 \
	glIslist.3 glLight.3 glLightModel.3 glLineStipple.3 \
	glLineWidth.3 glListBase.3 glLoadIdentity.3 glLoadMatrix.3 \
	glLoadName.3 glLogicOp.3 glMap1.3 glMap2.3 glMapGrid.3 \
	glMaterial.3 glMatrixMode.3 glMinmax.3 glMultMatrix.3 \
	glMultiTexCoordARB.3 glNewList.3 glNormal.3 glNormalPointer.3 \
	glOrtho.3 glPassThrough.3 glPixelMap.3 glPixelStore.3 \
	glPixelTransfer.3 glPixelZoom.3 glPointSize.3 glPolygonMode.3 \
	glPolygonOffset.3 glPolygonStipple.3 glPrioritizeTextures.3 \
	glPushAttrib.3 glPushClientAttrib.3 glPushMatrix.3 glPushName.3 \
	glRasterPos.3 glReadBuffer.3 glReadPixels.3 glRect.3 \
	glRenderMode.3 glResetHistogram.3 glResetMinmax.3 glRotate.3 \
	glScale.3 glScissor.3 glSelectBuffer.3 glSeparableFilter2D.3 \
	glShadeModel.3 glStencilFunc.3 glStencilMask.3 glStencilOp.3 \
	glTexCoord.3 glTexCoordPointer.3 glTexEnv.3 glTexGen.3 \
	glTexImage1D.3 glTexImage2D.3 glTexImage3D.3 glTexParameter.3 \
	glTexSubImage1D.3 glTexSubImage2D.3 glTexSubImage3D.3 \
	glTranslate.3 glVertex.3 glVertexPointer.3 glViewport.3

MLINKS=	glBegin.3 glEnd.3 \
	glColor.3 glColor3b.3 \
	glColor.3 glColor3d.3 \
	glColor.3 glColor3f.3 \
	glColor.3 glColor3i.3 \
	glColor.3 glColor3s.3 \
	glColor.3 glColor3ub.3 \
	glColor.3 glColor3ui.3 \
	glColor.3 glColor3us.3 \
	glColor.3 glColor4b.3 \
	glColor.3 glColor4d.3 \
	glColor.3 glColor4f.3 \
	glColor.3 glColor4i.3 \
	glColor.3 glColor4s.3 \
	glColor.3 glColor4ub.3 \
	glColor.3 glColor4ui.3 \
	glColor.3 glColor4us.3 \
	glColor.3 glColor3bv.3 \
	glColor.3 glColor3dv.3 \
	glColor.3 glColor3fv.3 \
	glColor.3 glColor3iv.3 \
	glColor.3 glColor3sv.3 \
	glColor.3 glColor3ubv.3 \
	glColor.3 glColor3uiv.3 \
	glColor.3 glColor3usv.3 \
	glColor.3 glColor4bv.3 \
	glColor.3 glColor4dv.3 \
	glColor.3 glColor4fv.3 \
	glColor.3 glColor4iv.3 \
	glColor.3 glColor4sv.3 \
	glColor.3 glColor4ubv.3 \
	glColor.3 glColor4uiv.3 \
	glColor.3 glColor4usv.3 \
	glColorTableParameter.3 glColorTableParameterfv.3 \
	glColorTableParameter.3 glColorTableParameteriv.3 \
	glEdgeFlag.3 glEdgeFlagv.3 \
	glEnable.3 glDisable.3 \
	glEnableClientState.3 glDisableClientState.3 \
	glEvalCoord.3 glEvalCoord1d.3 \
	glEvalCoord.3 glEvalCoord2d.3 \
	glEvalCoord.3 glEvalCoord2f.3 \
	glEvalCoord.3 glEvalCoord1dv.3 \
	glEvalCoord.3 glEvalCoord1fv.3 \
	glEvalCoord.3 glEvalCoord2dv.3 \
	glEvalCoord.3 glEvalCoord2fv.3 \
	glEvalMesh.3 glEvalMesh1.3 \
	glEvalMesh.3 glEvalMesh2.3 \
	glEvalPoint.3 glEvalPoint1.3 \
	glEvalPoint.3 glEvalPoint2.3 \
	glFog.3 glFogf.3 \
	glFog.3 glFogi.3 \
	glFog.3 glFogfv.3 \
	glFog.3 glFogiv.3 \
	glGet.3 glGetBooleanv.3 \
	glGet.3 glGetDoublev.3 \
	glGet.3 glGetFloatv.3 \
	glGet.3 glGetIntegerv.3 \
	glGetColorTableParameter.3 glGetColorTableParameterfv.3 \
	glGetColorTableParameter.3 glGetColorTableParameteriv.3 \
	glGetLight.3 glGetLightfv.3 \
	glGetLight.3 glGetLightiv.3 \
	glGetMap.3 glGetMapdv.3 \
	glGetMap.3 glGetMapfv.3 \
	glGetMap.3 glGetMapiv.3 \
	glGetMaterial.3 glGetMaterialfv.3 \
	glGetMaterial.3 glGetMaterialiv.3 \
	glGetPixelMap.3 glGetPixelMapfv.3 \
	glGetPixelMap.3 glGetPixelMapuiv.3 \
	glGetPixelMap.3 glGetPixelMapusv.3 \
	glGetTexEnv.3 glGetTexEnvfv.3 \
	glGetTexEnv.3 glGetTexEnviv.3 \
	glGetTexGen.3 glGetTexGendv.3 \
	glGetTexGen.3 glGetTexGenfv.3 \
	glGetTexGen.3 glGetTexGeniv.3 \
	glGetTexLevelParameter.3 glGetTexLevelParameterfv.3 \
	glGetTexLevelParameter.3 glGetTexLevelParameteriv.3 \
	glGetTexParameter.3 glGetTexParameterfv.3 \
	glGetTexParameter.3 glGetTexParameteriv.3 \
	glIndex.3 glIndexd.3 \
	glIndex.3 glIndexf.3 \
	glIndex.3 glIndexi.3 \
	glIndex.3 glIndexs.3 \
	glIndex.3 glIndexub.3 \
	glIndex.3 glIndexdv.3 \
	glIndex.3 glIndexfv.3 \
	glIndex.3 glIndexiv.3 \
	glIndex.3 glIndexsv.3 \
	glIndex.3 glIndexubv.3 \
	glLight.3 glLightf.3 \
	glLight.3 glLighti.3 \
	glLight.3 glLightfv.3 \
	glLight.3 glLightiv.3 \
	glLightModel.3 glLightModelf.3 \
	glLightModel.3 glLightModeli.3 \
	glLightModel.3 glLightModelfv.3 \
	glLightModel.3 glLightModeliv.3 \
	glLoadMatrix.3 glLoadMatrixd.3 \
	glLoadMatrix.3 glLoadMatrixf.3 \
	glMap1.3 glMap1d.3 \
	glMap1.3 glMap1f.3 \
	glMap2.3 glMap2d.3 \
	glMap2.3 glMap2f.3 \
	glMapGrid.3 glMapGrid1d.3 \
	glMapGrid.3 glMapGrid1f.3 \
	glMapGrid.3 glMapGrid2d.3 \
	glMapGrid.3 glMapGrid2f.3 \
	glMaterial.3 glMaterialf.3 \
	glMaterial.3 glMateriali.3 \
	glMaterial.3 glMaterialfv.3 \
	glMaterial.3 glMaterialiv.3 \
	glMultMatrix.3 glMultMatrixd.3 \
	glMultMatrix.3 glMultMatrixf.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1dARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1dvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1fARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1fvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1iARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1ivARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1sARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord1svARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2dARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2dvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2fARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2fvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2iARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2ivARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2sARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord2svARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3dARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3dvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3fARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3fvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3iARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3ivARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3sARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord3svARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4dARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4dvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4fARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4fvARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4iARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4ivARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4sARB.3 \
	glMultiTexCoordARB.3 glMultiTexCoord4svARB.3 \
	glNewList.3 glEndList.3 \
	glNormal.3 glNormal3b.3 \
	glNormal.3 glNormal3d.3 \
	glNormal.3 glNormal3f.3 \
	glNormal.3 glNormal3i.3 \
	glNormal.3 glNormal3s.3 \
	glNormal.3 glNormal3bv.3 \
	glNormal.3 glNormal3dv.3 \
	glNormal.3 glNormal3fv.3 \
	glNormal.3 glNormal3iv.3 \
	glNormal.3 glNormal3sv.3 \
	glPixelMap.3 glPixelMapfv.3 \
	glPixelMap.3 glPixelMapuiv.3 \
	glPixelMap.3 glPixelMapusv.3 \
	glPixelStore.3 glPixelStoref.3 \
	glPixelStore.3 glPixelStorei.3 \
	glPixelTransfer.3 glPixelTransferf.3 \
	glPixelTransfer.3 glPixelTransferi.3 \
	glPushAttrib.3 glPopAttrib.3 \
	glPushClientAttrib.3 glPopClientAttrib.3 \
	glPushMatrix.3 glPopMatrix.3 \
	glPushName.3 glPopName.3 \
	glRasterPos.3 glRasterPos2d.3 \
	glRasterPos.3 glRasterPos2f.3 \
	glRasterPos.3 glRasterPos2i.3 \
	glRasterPos.3 glRasterPos2s.3 \
	glRasterPos.3 glRasterPos3d.3 \
	glRasterPos.3 glRasterPos3f.3 \
	glRasterPos.3 glRasterPos3i.3 \
	glRasterPos.3 glRasterPos3s.3 \
	glRasterPos.3 glRasterPos4d.3 \
	glRasterPos.3 glRasterPos4f.3 \
	glRasterPos.3 glRasterPos4i.3 \
	glRasterPos.3 glRasterPos4s.3 \
	glRasterPos.3 glRasterPos2dv.3 \
	glRasterPos.3 glRasterPos2fv.3 \
	glRasterPos.3 glRasterPos2iv.3 \
	glRasterPos.3 glRasterPos2sv.3 \
	glRasterPos.3 glRasterPos3dv.3 \
	glRasterPos.3 glRasterPos3fv.3 \
	glRasterPos.3 glRasterPos3iv.3 \
	glRasterPos.3 glRasterPos3sv.3 \
	glRasterPos.3 glRasterPos4dv.3 \
	glRasterPos.3 glRasterPos4fv.3 \
	glRasterPos.3 glRasterPos4iv.3 \
	glRasterPos.3 glRasterPos4sv.3 \
	glRect.3 glRectd.3 \
	glRect.3 glRectf.3 \
	glRect.3 glRecti.3 \
	glRect.3 glRects.3 \
	glRect.3 glRectdv.3 \
	glRect.3 glRectfv.3 \
	glRect.3 glRectiv.3 \
	glRect.3 glRectsv.3 \
	glRotate.3 glRotated.3 \
	glRotate.3 glRotatef.3 \
	glScale.3 glScaled.3 \
	glScale.3 glScalef.3 \
	glTexCoord.3 glTexCoord1d.3 \
	glTexCoord.3 glTexCoord1f.3 \
	glTexCoord.3 glTexCoord1i.3 \
	glTexCoord.3 glTexCoord1s.3 \
	glTexCoord.3 glTexCoord2d.3 \
	glTexCoord.3 glTexCoord2f.3 \
	glTexCoord.3 glTexCoord2i.3 \
	glTexCoord.3 glTexCoord2s.3 \
	glTexCoord.3 glTexCoord3d.3 \
	glTexCoord.3 glTexCoord3f.3 \
	glTexCoord.3 glTexCoord3i.3 \
	glTexCoord.3 glTexCoord3s.3 \
	glTexCoord.3 glTexCoord4d.3 \
	glTexCoord.3 glTexCoord4f.3 \
	glTexCoord.3 glTexCoord4i.3 \
	glTexCoord.3 glTexCoord4s.3 \
	glTexCoord.3 glTexCoord1dv.3 \
	glTexCoord.3 glTexCoord1fv.3 \
	glTexCoord.3 glTexCoord1iv.3 \
	glTexCoord.3 glTexCoord1sv.3 \
	glTexCoord.3 glTexCoord2dv.3 \
	glTexCoord.3 glTexCoord2fv.3 \
	glTexCoord.3 glTexCoord2iv.3 \
	glTexCoord.3 glTexCoord2sv.3 \
	glTexCoord.3 glTexCoord3dv.3 \
	glTexCoord.3 glTexCoord3fv.3 \
	glTexCoord.3 glTexCoord3iv.3 \
	glTexCoord.3 glTexCoord3sv.3 \
	glTexCoord.3 glTexCoord4dv.3 \
	glTexCoord.3 glTexCoord4fv.3 \
	glTexCoord.3 glTexCoord4iv.3 \
	glTexCoord.3 glTexCoord4sv.3 \
	glTexEnv.3 glTexEnvf.3 \
	glTexEnv.3 glTexEnvi.3 \
	glTexEnv.3 glTexEnvfv.3 \
	glTexEnv.3 glTexEnviv.3 \
	glTexGen.3 glTexGend.3 \
	glTexGen.3 glTexGenf.3 \
	glTexGen.3 glTexGeni.3 \
	glTexGen.3 glTexGendv.3 \
	glTexGen.3 glTexGenfv.3 \
	glTexGen.3 glTexGeniv.3 \
	glTexParameter.3 glTexParameterf.3 \
	glTexParameter.3 glTexParameteri.3 \
	glTexParameter.3 glTexParameterfv.3 \
	glTexParameter.3 glTexParameteriv.3 \
	glTranslate.3 glTranslated.3 \
	glTranslate.3 glTranslatef.3 \
	glVertex.3 glVertex2d.3 \
	glVertex.3 glVertex2f.3 \
	glVertex.3 glVertex2i.3 \
	glVertex.3 glVertex2s.3 \
	glVertex.3 glVertex3d.3 \
	glVertex.3 glVertex3f.3 \
	glVertex.3 glVertex3i.3 \
	glVertex.3 glVertex3s.3 \
	glVertex.3 glVertex4d.3 \
	glVertex.3 glVertex4f.3 \
	glVertex.3 glVertex4i.3 \
	glVertex.3 glVertex4s.3 \
	glVertex.3 glVertex2dv.3 \
	glVertex.3 glVertex2fv.3 \
	glVertex.3 glVertex2iv.3 \
	glVertex.3 glVertex2sv.3 \
	glVertex.3 glVertex3dv.3 \
	glVertex.3 glVertex3fv.3 \
	glVertex.3 glVertex3iv.3 \
	glVertex.3 glVertex3sv.3 \
	glVertex.3 glVertex4dv.3 \
	glVertex.3 glVertex4fv.3 \
	glVertex.3 glVertex4iv.3 \
	glVertex.3 glVertex4sv.3

BUILDSYMLINKS=\
	accum.3gl glAccum.man \
	activetextureARB.3gl glActiveTextureARB.man \
	alphafunc.3gl glAlphaFunc.man \
	aretexturesresident.3gl glAreTexturesResident.man \
	arrayelement.3gl glArrayElement.man \
	begin.3gl glBegin.man \
	bindtexture.3gl glBindTexture.man \
	bitmap.3gl glBitmap.man \
	blendcolor.3gl glBlendColor.man \
	blendequation.3gl glBlendEquation.man \
	blendfunc.3gl glBlendFunc.man \
	calllist.3gl glCallList.man \
	calllists.3gl glCallLists.man \
	clear.3gl glClear.man \
	clearaccum.3gl glClearAccum.man \
	clearcolor.3gl glClearColor.man \
	cleardepth.3gl glClearDepth.man \
	clearindex.3gl glClearIndex.man \
	clearstencil.3gl glClearStencil.man \
	clientactivetextureARB.3gl glClientActiveTextureARB.man \
	clipplane.3gl glClipPlane.man \
	color.3gl glColor.man \
	colormask.3gl glColorMask.man \
	colormaterial.3gl glColorMaterial.man \
	colorpointer.3gl glColorPointer.man \
	colorsubtable.3gl glColorSubTable.man \
	colortable.3gl glColorTable.man \
	colortableparameter.3gl glColorTableParameter.man \
	convolutionfilter1d.3gl glConvolutionFilter1D.man \
	convolutionfilter2d.3gl glConvolutionFilter2D.man \
	convolutionparameter.3gl glConvolutionParameter.man \
	copycolorsubtable.3gl glCopyColorSubTable.man \
	copycolortable.3gl glCopyColorTable.man \
	copyconvolutionfilter1d.3gl glCopyConvolutionFilter1D.man \
	copyconvolutionfilter2d.3gl glCopyConvolutionFilter2D.man \
	copypixels.3gl glCopyPixels.man \
	copyteximage1d.3gl glCopyTexImage1D.man \
	copyteximage2d.3gl glCopyTexImage2D.man \
	copytexsubimage1d.3gl glCopyTexSubImage1D.man \
	copytexsubimage2d.3gl glCopyTexSubImage2D.man \
	copytexsubimage3d.3gl glCopyTexSubImage3D.man \
	cullface.3gl glCullFace.man \
	deletelists.3gl glDeleteLists.man \
	deletetextures.3gl glDeleteTextures.man \
	depthfunc.3gl glDepthFunc.man \
	depthmask.3gl glDepthMask.man \
	depthrange.3gl glDepthRange.man \
	drawarrays.3gl glDrawArrays.man \
	drawbuffer.3gl glDrawBuffer.man \
	drawelements.3gl glDrawElements.man \
	drawpixels.3gl glDrawPixels.man \
	drawrangeelements.3gl glDrawRangeElements.man \
	edgeflag.3gl glEdgeFlag.man \
	edgeflagpointer.3gl glEdgeFlagPointer.man \
	enable.3gl glEnable.man \
	enableclientstate.3gl glEnableClientState.man \
	evalcoord.3gl glEvalCoord.man \
	evalmesh.3gl glEvalMesh.man \
	evalpoint.3gl glEvalPoint.man \
	feedbackbuffer.3gl glFeedbackBuffer.man \
	finish.3gl glFinish.man \
	flush.3gl glFlush.man \
	fog.3gl glFog.man \
	frontface.3gl glFrontFace.man \
	frustum.3gl glFrustum.man \
	genlists.3gl glGenLists.man \
	gentextures.3gl glGenTextures.man \
	get.3gl glGet.man \
	getclipplane.3gl glGetClipPlane.man \
	getcolortable.3gl glGetColorTable.man \
	getcolortableparameter.3gl glGetColorTableParameter.man \
	getconvolutionfilter.3gl glGetConvolutionFilter.man \
	getconvolutionparameter.3gl glGetConvolutionParameter.man \
	geterror.3gl glGetError.man \
	gethistogram.3gl glGetHistogram.man \
	gethistogramparameter.3gl glGetHistogramParameter.man \
	getlight.3gl glGetLight.man \
	getmap.3gl glGetMap.man \
	getmaterial.3gl glGetMaterial.man \
	getminmax.3gl glGetMinmax.man \
	getminmaxparameter.3gl glGetMinmaxParameter.man \
	getpixelmap.3gl glGetPixelMap.man \
	getpointerv.3gl glGetPointerv.man \
	getpolygonstipple.3gl glGetPolygonStipple.man \
	getseparablefilter.3gl glGetSeparableFilter.man \
	getstring.3gl glGetString.man \
	gettexenv.3gl glGetTexEnv.man \
	gettexgen.3gl glGetTexGen.man \
	getteximage.3gl glGetTexImage.man \
	gettexlevelparameter.3gl glGetTexLevelParameter.man \
	gettexparameter.3gl glGetTexParameter.man \
	hint.3gl glHint.man \
	histogram.3gl glHistogram.man \
	index.3gl glIndex.man \
	indexmask.3gl glIndexMask.man \
	indexpointer.3gl glIndexPointer.man \
	initnames.3gl glInitNames.man \
	interleavedarrays.3gl glInterleavedArrays.man \
	isenabled.3gl glIsEnabled.man \
	islist.3gl glIslist.man \
	istexture.3gl glIsTexture.man \
	light.3gl glLight.man \
	lightmodel.3gl glLightModel.man \
	linestipple.3gl glLineStipple.man \
	linewidth.3gl glLineWidth.man \
	listbase.3gl glListBase.man \
	loadidentity.3gl glLoadIdentity.man \
	loadmatrix.3gl glLoadMatrix.man \
	loadname.3gl glLoadName.man \
	logicop.3gl glLogicOp.man \
	map1.3gl glMap1.man \
	map2.3gl glMap2.man \
	mapgrid.3gl glMapGrid.man \
	material.3gl glMaterial.man \
	matrixmode.3gl glMatrixMode.man \
	minmax.3gl glMinmax.man \
	multitexcoordARB.3gl glMultiTexCoordARB.man \
	multmatrix.3gl glMultMatrix.man \
	newlist.3gl glNewList.man \
	normal.3gl glNormal.man \
	normalpointer.3gl glNormalPointer.man \
	ortho.3gl glOrtho.man \
	passthrough.3gl glPassThrough.man \
	pixelmap.3gl glPixelMap.man \
	pixelstore.3gl glPixelStore.man \
	pixeltransfer.3gl glPixelTransfer.man \
	pixelzoom.3gl glPixelZoom.man \
	pointsize.3gl glPointSize.man \
	polygonmode.3gl glPolygonMode.man \
	polygonoffset.3gl glPolygonOffset.man \
	polygonstipple.3gl glPolygonStipple.man \
	prioritizetextures.3gl glPrioritizeTextures.man \
	pushattrib.3gl glPushAttrib.man \
	pushclientattrib.3gl glPushClientAttrib.man \
	pushmatrix.3gl glPushMatrix.man \
	pushname.3gl glPushName.man \
	rasterpos.3gl glRasterPos.man \
	readbuffer.3gl glReadBuffer.man \
	readpixels.3gl glReadPixels.man \
	rect.3gl glRect.man \
	rendermode.3gl glRenderMode.man \
	resethistogram.3gl glResetHistogram.man \
	resetminmax.3gl glResetMinmax.man \
	rotate.3gl glRotate.man \
	scale.3gl glScale.man \
	scissor.3gl glScissor.man \
	selectbuffer.3gl glSelectBuffer.man \
	separablefilter2d.3gl glSeparableFilter2D.man \
	shademodel.3gl glShadeModel.man \
	stencilfunc.3gl glStencilFunc.man \
	stencilmask.3gl glStencilMask.man \
	stencilop.3gl glStencilOp.man \
	texcoord.3gl glTexCoord.man \
	texcoordpointer.3gl glTexCoordPointer.man \
	texenv.3gl glTexEnv.man \
	texgen.3gl glTexGen.man \
	teximage1d.3gl glTexImage1D.man \
	teximage2d.3gl glTexImage2D.man \
	teximage3d.3gl glTexImage3D.man \
	texparameter.3gl glTexParameter.man \
	texsubimage1d.3gl glTexSubImage1D.man \
	texsubimage2d.3gl glTexSubImage2D.man \
	texsubimage3d.3gl glTexSubImage3D.man \
	translate.3gl glTranslate.man \
	vertex.3gl glVertex.man \
	vertexpointer.3gl glVertexPointer.man \
	viewport.3gl glViewport.man

.PATH:  ${X11SRCDIR.xc}/doc/man/GL/gl

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.man.mk>
