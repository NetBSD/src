#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $

.include <bsd.own.mk>

LIB=	oldX
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XCrAssoc.c XDelAssoc.c XDestAssoc.c XDraw.c XLookAssoc.c XMakeAssoc.c

INCS=	X10.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11

#LM#	LDADD+=	-lX11
#LM#	DPADD+=	${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
