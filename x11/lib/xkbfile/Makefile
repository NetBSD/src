#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:59 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	xkbfile
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	cout.c maprules.c srvmisc.c xkbatom.c xkbbells.c xkbconfig.c \
	xkbdraw.c xkberrs.c xkbmisc.c xkbout.c xkbtext.c xkmout.c \
	xkmread.c

INCS=	XKBbells.h XKBconfig.h XKBfile.h XKBrules.h XKM.h XKMformat.h
INCSDIR=${X11INCDIR}/X11/extensions

.include <bsd.x11.mk>
.include <bsd.lib.mk>
