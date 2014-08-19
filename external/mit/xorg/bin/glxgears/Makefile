#	$NetBSD: Makefile,v 1.2.28.1 2014/08/20 00:00:08 tls Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/src/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
