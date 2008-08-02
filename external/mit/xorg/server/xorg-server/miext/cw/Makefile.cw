#	$NetBSD: Makefile.cw,v 1.1 2008/08/02 04:32:00 mrg Exp $

LIB=	cw

.PATH:	${X11SRCDIR.xorg-server}/miext/${LIB}
SRCS+=	cw.c cw_ops.c cw_render.c


CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION} \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		${X11FLAGS.DIX} ${X11INCS.DIX}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
