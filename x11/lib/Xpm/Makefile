#	$NetBSD: Makefile,v 1.6 2004/08/05 01:28:48 lukem Exp $

.include <bsd.own.mk>

LIB=	Xpm
.PATH:	${X11SRCDIR.xc}/extras/Xpm/lib

SRCS=	Attrib.c CrBufFrI.c CrBufFrP.c CrDatFrI.c CrDatFrP.c \
	CrIFrBuf.c CrIFrDat.c CrIFrP.c CrPFrBuf.c CrPFrDat.c CrPFrI.c \
	Image.c Info.c RdFToBuf.c RdFToDat.c RdFToI.c RdFToP.c \
	WrFFrBuf.c WrFFrDat.c WrFFrI.c WrFFrP.c create.c data.c \
	hashtab.c misc.c parse.c rgb.c scan.c

INCS=	xpm.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=-DHAS_SNPRINTF

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
