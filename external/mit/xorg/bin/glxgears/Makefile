#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	glxgears
MAN=	# defined

CPPFLAGS+=${X11FLAGS.THREADS}

LDADD+=	-lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/progs/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
