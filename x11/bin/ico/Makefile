#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:05 lukem Exp $

.include <bsd.own.mk>

PROG=	ico

CPPFLAGS+=-DMULTITHREAD ${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION}

LDADD+=	-lXext -lX11 -lpthread -lm
DPADD+=	${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
