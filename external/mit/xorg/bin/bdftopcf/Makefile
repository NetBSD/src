#	$NetBSD: Makefile,v 1.5 2018/03/11 10:48:38 mrg Exp $

.include <bsd.own.mk>

PROG=	bdftopcf
SRCS=	atom.c \
	bdfread.c \
	bdfutils.c \
	bitmap.c \
	bitmaputil.c \
	bufio.c \
	defaults.c \
	fileio.c \
	filewr.c \
	fontaccel.c \
	fontink.c \
	pcfwrite.c \
	private.c \
	utilbitmap.c \
	bdftopcf.c

LDADD+=	-lm -lz -lbz2
DPADD+=	${LIBM} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
