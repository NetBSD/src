#	$NetBSD: Makefile,v 1.4 2003/09/30 06:41:31 lukem Exp $

.include <bsd.own.mk>

LIB=	oldX
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XCrAssoc.c XDelAssoc.c XDestAssoc.c XDraw.c XLookAssoc.c XMakeAssoc.c

INCS=	X10.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	-lX11
DPADD+=	${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
