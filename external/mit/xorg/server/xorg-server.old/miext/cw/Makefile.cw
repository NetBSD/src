#	$NetBSD: Makefile.cw,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

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
