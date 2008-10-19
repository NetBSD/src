#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:40:57 haad Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	glxinfo

CPPFLAGS+=${X11FLAGS.THREADS} -DDO_GLU

LDADD+=	-lGLU -lGL -lXext -lX11 -lpthread -lstdc++ -lm
DPADD+=	${LIBGLU} ${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD}
DPADD+=	${LIBSTDCXX} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/progs/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
