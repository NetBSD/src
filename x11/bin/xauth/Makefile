#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:13 lukem Exp $

.include <bsd.own.mk>

PROG=	xauth
SRCS=	xauth.c gethost.c process.c parsedpy.c

CPPFLAGS.gethost.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.parsedpy.c=	${X11FLAGS.CONNECTION}

LDADD+=	-lXau -lXmuu -lXext -lX11
DPADD+=	${LIBXAU} ${LIBXMUU} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
