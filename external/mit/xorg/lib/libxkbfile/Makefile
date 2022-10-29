#	$NetBSD: Makefile,v 1.6 2022/10/29 21:06:15 mrg Exp $

.include <bsd.own.mk>

LIB=	xkbfile
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	cout.c maprules.c srvmisc.c xkbatom.c xkbbells.c xkbconfig.c \
	xkbdraw.c xkberrs.c xkbmisc.c xkbout.c xkbtext.c xkmout.c \
	xkmread.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions
CPPFLAGS+=	-DHAVE_UNLOCKED_STDIO -DHAVE_STRNDUP -DHAVE_STRCASECMP

INCS=	XKBbells.h XKBconfig.h XKBfile.h XKBrules.h XKM.h XKMformat.h
INCSDIR=${X11INCDIR}/X11/extensions

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

PKGCONFIG=	xkbfile

COPTS.xkbatom.c=	${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
