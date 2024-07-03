#	$NetBSD: Makefile,v 1.6 2024/07/03 09:30:22 mrg Exp $

.include <bsd.own.mk>

PROG=	bitmap
SRCS=	BitEdit.c CutPaste.c Graphics.c ReqMach.c Bitmap.c Dialog.c Handlers.c

.PATH:	${X11SRCDIR.bitmap}/man
MLINKS=	bitmap.1 bmtoa.1 bitmap.1 atobm.1

FILES=	Dashes Down Excl FlipHoriz FlipVert Fold Left Right RotateLeft \
	RotateRight Stipple Term Up
FILESDIR=${X11INCDIR}/X11/bitmaps

.include "Makefile.common"

SUBDIR=	bmtoa atobm app-defaults

.PATH:	${X11SRCDIR.bitmap} ${X11SRCDIR.bitmap}/bitmaps

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
