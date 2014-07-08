#	$NetBSD: Makefile,v 1.3 2014/07/08 22:29:12 riastradh Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/src/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
