#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:39 lukem Exp $

.include <bsd.own.mk>

LIB=	Xrandr
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xrandr.c

INCS=	Xrandr.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11/extensions

MAN=	Xrandr.3

#LM#	LDADD+=	-lXrender -lXext -lX11
#LM#	DPADD+=	${LIBXRENDER} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
