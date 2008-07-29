#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

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

CPPFLAGS+=-DHAS_SNPRINTF
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
