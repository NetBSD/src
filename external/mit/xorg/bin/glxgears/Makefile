#	$NetBSD: Makefile,v 1.3.2.1 2015/01/05 21:23:51 martin Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.mesa-demos}/src/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
