#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	glxinfo
MAN=	# defined

CPPFLAGS+=${X11FLAGS.THREADS} -DDO_GLU

LDADD+=	-lGLU -lGL -lXext -lX11 -lpthread -lstdc++ -lm
DPADD+=	${LIBGLU} ${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD}
DPADD+=	${LIBSTDCXX} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/progs/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
