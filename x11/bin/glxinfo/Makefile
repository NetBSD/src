#	$NetBSD: Makefile,v 1.3.60.1 2014/08/20 00:05:19 tls Exp $

.include <bsd.own.mk>

.if ${MKPIC} == "no" || ${LDSTATIC:U} != ""
PROG_CXX=	glxinfo
.else
PROG=		glxinfo
.endif

CPPFLAGS+=${X11FLAGS.THREADS} -DDO_GLU

LDADD+=	-lGLU -lGL -lXext -lX11 -lpthread -lm
DPADD+=	${LIBGLU} ${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
