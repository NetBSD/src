#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:39 lukem Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c dsimple.c

CPPFLAGS+=-I${X11SRCDIR.xc}/programs/xlsfonts
.PATH:	${X11SRCDIR.xc}/programs/xlsfonts

LDADD+=	-lXmuu -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
