#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	bitmap
SRCS=	BitEdit.c CutPaste.c Graphics.c ReqMach.c Bitmap.c Dialog.c Handlers.c

MLINKS=	bitmap.1 bmtoa.1 bitmap.1 atobm.1

INCS=	Dashes Down Excl FlipHoriz FlipVert Fold Left Right RotateLeft \
	RotateRight Stipple Term Up
INCSDIR=${X11INCDIR}/X11/bitmaps

APPDEFS=Bitmap.ad Bitmap-color.ad

LNFILES=Bitmap-co.ad Bitmap-color.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

SUBDIR=	bmtoa atobm

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
