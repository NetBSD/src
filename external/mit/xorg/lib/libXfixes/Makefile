#	$NetBSD: Makefile,v 1.6 2021/07/11 00:08:06 mrg Exp $

.include <bsd.own.mk>

LIB=	Xfixes

XFIXESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XFIXESDIR}/src
.PATH:		${XFIXESDIR}/man
.PATH:		${XFIXESDIR}/include/X11/extensions
SRCS=		Cursor.c Disconnect.c Region.c SaveSet.c Selection.c Xfixes.c

INCS=	Xfixes.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	-DHAVE__XEATDATAWORDS 

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

MAN=	Xfixes.3

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
