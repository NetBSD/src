#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 14:09:36 yamt Exp $

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

.PATH:	${X11SRCDIR.MesaDemos}/progs/xdemos

.include <bsd.x11.mk>
.include <bsd.prog.mk>
