#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:45:05 lukem Exp $

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
