#	$NetBSD: Makefile,v 1.2.28.1 2014/08/20 00:00:08 tls Exp $

NOMAN=	# defined

.include <bsd.own.mk>

.if ${MKPIC} == "no" || ${LDSTATIC:U} != ""
PROG_CXX=	glxinfo
.else
PROG=		glxinfo
.endif

CPPFLAGS+=${X11FLAGS.THREADS} -DDO_GLU

LDADD+=	-lGLU -lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGLU} ${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.MesaDemos}/src/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
