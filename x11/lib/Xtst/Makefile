#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:20 lukem Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
