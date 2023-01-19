#	$NetBSD: Makefile,v 1.4 2023/01/19 05:31:34 mrg Exp $

USE_FORT?=	yes # data-driven bugs?

.include <bsd.own.mk>

LIB=	Xpm
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11

SRCS=	Attrib.c CrBufFrI.c CrBufFrP.c CrDatFrI.c CrDatFrP.c \
	CrIFrBuf.c CrIFrDat.c CrIFrP.c CrPFrBuf.c CrPFrDat.c CrPFrI.c \
	Image.c Info.c RdFToBuf.c RdFToDat.c RdFToI.c RdFToP.c \
	WrFFrBuf.c WrFFrDat.c WrFFrI.c WrFFrP.c create.c data.c \
	hashtab.c misc.c parse.c rgb.c scan.c

INCS=	xpm.h
INCSDIR=${X11INCDIR}/X11

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include "xpm.mk"

.if 0 # not yet
MAN=	\
	XpmAttributesSize.3 \
	XpmCreateBuffer.3 \
	XpmCreateBufferFromImage.3 \
	XpmCreateBufferFromPixmap.3 \
	XpmCreateBufferFromXpmImage.3 \
	XpmCreateData.3 \
	XpmCreateDataFromImage.3 \
	XpmCreateDataFromPixmap.3 \
	XpmCreateDataFromXpmImage.3 \
	XpmCreateImage.3 \
	XpmCreateImageFromBuffer.3 \
	XpmCreateImageFromData.3 \
	XpmCreateImageFromXpmImage.3 \
	XpmCreatePixmap.3 \
	XpmCreatePixmapFromBuffer.3 \
	XpmCreatePixmapFromData.3 \
	XpmCreatePixmapFromXpmImage.3 \
	XpmCreateXpmImage.3 \
	XpmCreateXpmImageFromBuffer.3 \
	XpmCreateXpmImageFromData.3 \
	XpmCreateXpmImageFromImage.3 \
	XpmCreateXpmImageFromPixmap.3 \
	XpmFree.3 \
	XpmFreeAttributes.3 \
	XpmFreeExtensions.3 \
	XpmFreeXpmImage.3 \
	XpmFreeXpmInfo.3 \
	XpmGetErrorString.3 \
	XpmLibraryVersion.3 \
	XpmMisc.3 \
	XpmRead.3 \
	XpmReadFileToBuffer.3 \
	XpmReadFileToData.3 \
	XpmReadFileToImage.3 \
	XpmReadFileToPixmap.3 \
	XpmReadFileToXpmImage.3 \
	XpmWrite.3 \
	XpmWriteFileFromBuffer.3 \
	XpmWriteFileFromImage.3 \
	XpmWriteFileFromPixmap.3 \
	XpmWriteFileFromXpmImage.3
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
