#	$NetBSD: Makefile,v 1.3 2011/08/11 23:15:35 joerg Exp $

.include <bsd.own.mk>

PROG=	bitmap
SRCS=	BitEdit.c CutPaste.c Graphics.c ReqMach.c Bitmap.c Dialog.c Handlers.c

.PATH:	${X11SRCDIR.bitmap}/man
MLINKS=	bitmap.1 bmtoa.1 bitmap.1 atobm.1

FILES=	Dashes Down Excl FlipHoriz FlipVert Fold Left Right RotateLeft \
	RotateRight Stipple Term Up
FILESDIR=${X11INCDIR}/X11/bitmaps

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

SUBDIR=	bmtoa atobm app-defaults

.PATH:	${X11SRCDIR.bitmap}

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
