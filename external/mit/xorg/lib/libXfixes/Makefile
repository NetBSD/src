#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:04 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xfixes

XFIXESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XFIXESDIR}/src
.PATH:		${XFIXESDIR}/include/X11/extensions
SRCS=		Cursor.c Region.c SaveSet.c Selection.c Xfixes.c

INCS=	Xfixes.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
