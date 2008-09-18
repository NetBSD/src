#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:26 wrstuden Exp $

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
