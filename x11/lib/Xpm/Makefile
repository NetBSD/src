#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:16 lukem Exp $

.include <bsd.own.mk>

LIB=	Xpm
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/extras/Xpm/lib

SRCS=	Attrib.c CrBufFrI.c CrBufFrP.c CrDatFrI.c CrDatFrP.c \
	CrIFrBuf.c CrIFrDat.c CrIFrP.c CrPFrBuf.c CrPFrDat.c CrPFrI.c \
	Image.c Info.c RdFToBuf.c RdFToDat.c RdFToI.c RdFToP.c \
	WrFFrBuf.c WrFFrDat.c WrFFrI.c WrFFrP.c create.c data.c \
	hashtab.c misc.c parse.c rgb.c scan.c

INCS=	xpm.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=-DHAS_SNPRINTF

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
