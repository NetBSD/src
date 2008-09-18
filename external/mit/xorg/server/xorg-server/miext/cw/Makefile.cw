#	$NetBSD: Makefile.cw,v 1.3.2.2 2008/09/18 05:18:48 wrstuden Exp $

.PATH:          ${X11SRCDIR.xorg-server}/miext/cw
SRCS.cw=	cw.c cw_ops.c cw_render.c

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION} \
		${X11FLAGS.DIX} ${X11INCS.DIX}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg

.include <bsd.x11.mk>
LIBDIR= ${XMODULEDIR}
.include <bsd.lib.mk>
