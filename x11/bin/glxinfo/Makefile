#	$NetBSD: Makefile,v 1.3 2003/09/23 11:01:20 lukem Exp $

.include <bsd.own.mk>

PROG=	glxinfo

CPPFLAGS+=${X11FLAGS.THREADS} -DDO_GLU

LDADD+=	-lGLU -lGL -lXext -lX11 -lpthread -lstdc++ -lm
DPADD+=	${LIBGLU} ${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD}
DPADD+=	${LIBSTDCXX} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
