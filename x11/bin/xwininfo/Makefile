#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c dsimple.c

CPPFLAGS+=-I${X11SRCDIR.xc}/programs/xlsfonts
.PATH:	${X11SRCDIR.xc}/programs/xlsfonts

LDADD+=	-lXmuu -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
