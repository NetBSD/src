#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:13 lukem Exp $

#XTODO	xcursor.pc
#XTODO	xcursor-config

.include <bsd.own.mk>

LIB=	Xcursor
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	cursor.c display.c file.c library.c xlib.c

INCS=	Xcursor.h
INCSDIR=${X11INCDIR}/X11/Xcursor

MAN=	Xcursor.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DICONDIR=\"${X11LIBDIR}/icons\"

LDADD+=	-lXrender -lXext -lX11
DPADD+=	${LIBXRENDER} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
