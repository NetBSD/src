#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:09 lukem Exp $

.include <bsd.own.mk>

LIB=		GL

.PATH:		${X11SRCDIR.xc}/lib/GL/glx
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src
SRCS=		clientattrib.c compsize.c dispatch.c eval.c g_render.c \
		g_single.c g_vendpriv.c glapi.c glthread.c glxcmds.c \
		glxext.c indirect_init.c pixel.c pixelstore.c \
		render2.c renderpix.c single2.c singlepix.c vertarr.c xfont.c

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src \
		${X11FLAGS.THREADLIB} -DGLXEXT -DGLX_USE_MESA

LDADD+=		-lXext -lX11
DPADD+=		${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
