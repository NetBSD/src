#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:31 lukem Exp $

.include <bsd.own.mk>

PROG=	xlsatoms

LDADD+=	-lXmuu -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
