#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:54:24 lukem Exp $

.include <bsd.own.mk>

PROG=	ico

CPPFLAGS+=-DMULTITHREAD ${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION}

LDADD+=	-lXext -lX11 -lpthread -lm
DPADD+=	${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
