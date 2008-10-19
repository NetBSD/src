#	$NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:10 haad Exp $

.include <bsd.own.mk>

LIB=	Xfixes

XFIXESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XFIXESDIR}/src
.PATH:		${XFIXESDIR}/man
.PATH:		${XFIXESDIR}/include/X11/extensions
SRCS=		Cursor.c Region.c SaveSet.c Selection.c Xfixes.c

INCS=	Xfixes.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

MAN=	Xfixes.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
