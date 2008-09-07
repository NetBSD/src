#	$NetBSD: Makefile,v 1.2 2008/09/07 14:07:21 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxgears

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/progs/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
