#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:51 lukem Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
