#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:12 haad Exp $

.include <bsd.own.mk>

LIB=	xkbfile
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	cout.c maprules.c srvmisc.c xkbatom.c xkbbells.c xkbconfig.c \
	xkbdraw.c xkberrs.c xkbmisc.c xkbout.c xkbtext.c xkmout.c \
	xkmread.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

INCSDIR=${X11INCDIR}/X11/extensions

INCS=	XKBbells.h XKBconfig.h XKBfile.h XKBrules.h XKM.h XKMformat.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGCONFIG=	xkbfile

.include <bsd.x11.mk>
.include <bsd.lib.mk>
