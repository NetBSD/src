#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:46 lukem Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

#LM#	LDADD+=	-lXext -lX11
#LM#	DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
