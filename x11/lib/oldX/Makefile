#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:22 lukem Exp $

.include <bsd.own.mk>

LIB=	oldX
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XCrAssoc.c XDelAssoc.c XDestAssoc.c XDraw.c XLookAssoc.c XMakeAssoc.c

INCS=	X10.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11

LDADD+=	-lX11
DPADD+=	${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
