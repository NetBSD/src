#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:56 lukem Exp $

.include <bsd.own.mk>

MAN=	XShapeQueryExtension.3 XmbufQueryExtension.3 XeviQueryVersion.3

MLINKS=	XShapeQueryExtension.3 XShapeQueryVersion.3 \
	XShapeQueryExtension.3 XShapeCombineRegion.3 \
	XShapeQueryExtension.3 XShapeCombineRectangles.3 \
	XShapeQueryExtension.3 XShapeCombineMask.3 \
	XShapeQueryExtension.3 XShapeCombineShape.3 \
	XShapeQueryExtension.3 XShapeOffsetShape.3 \
	XShapeQueryExtension.3 XShapeQueryExtents.3 \
	XShapeQueryExtension.3 XShapeSelectInput.3 \
	XShapeQueryExtension.3 XShapeInputSelected.3 \
	XShapeQueryExtension.3 XShapeGetRectangles.3

MLINKS+=XmbufQueryExtension.3 XmbufGetVersion.3 \
	XmbufQueryExtension.3 XmbufCreateBuffers.3 \
	XmbufQueryExtension.3 XmbufDestroyBuffers.3 \
	XmbufQueryExtension.3 XmbufDisplayBuffers.3 \
	XmbufQueryExtension.3 XmbufGetWindowAttributes.3 \
	XmbufQueryExtension.3 XmbufChangeWindowAttributes.3 \
	XmbufQueryExtension.3 XmbufGetBufferAttributes.3 \
	XmbufQueryExtension.3 XmbufChangeBufferAttributes.3 \
	XmbufQueryExtension.3 XmbufGetScreenInfo.3 \
	XmbufQueryExtension.3 XmbufCreateStereoWindow.3

MLINKS+=XeviQueryVersion.3 XeviGetVisualInfo.3 \
	XeviQueryVersion.3 XeviQueryExtension.3

BUILDSYMLINKS=\
	XShape.man XShapeQueryExtension.man \
	Xmbuf.man XmbufQueryExtension.man \
	Xevi.man XeviQueryVersion.man

SUBDIR=	dbe

.PATH:  ${X11SRCDIR.xc}/doc/man/Xext

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
