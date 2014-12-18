#	$NetBSD: Makefile,v 1.4 2014/12/18 06:24:28 mrg Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.mesa-demos}/src/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
