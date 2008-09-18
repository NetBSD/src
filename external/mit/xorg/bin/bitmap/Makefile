#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:25 wrstuden Exp $

.include <bsd.own.mk>

PROG=	bitmap
SRCS=	BitEdit.c CutPaste.c Graphics.c ReqMach.c Bitmap.c Dialog.c Handlers.c

MLINKS=	bitmap.1 bmtoa.1 bitmap.1 atobm.1

FILES=	Dashes Down Excl FlipHoriz FlipVert Fold Left Right RotateLeft \
	RotateRight Stipple Term Up
FILESDIR=${X11INCDIR}/X11/bitmaps

APPDEFS=Bitmap.ad Bitmap-color.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

SUBDIR=	bmtoa atobm

.PATH:	${X11SRCDIR.bitmap}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
