#	$NetBSD: Makefile.cw,v 1.1.2.1 2004/11/15 09:12:43 rtr Exp $

LIB=	cw

.PATH:	${X11SRCDIR.xc}/programs/Xserver/miext/${LIB}
SRCS+=	cw.c cw_ops.c cw_render.c


CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/miext/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/composite \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/shadow \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/include

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
