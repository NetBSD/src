#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:21 lukem Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION}

LDADD+=	-lXmuu -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
