#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

LIB=		render
.PATH:		${X11SRCDIR.xc}/programs/Xserver/render

SRCS=		animcur.c filter.c glyph.c miglyph.c miindex.c \
		mipict.c mirect.c mitrap.c mitri.c picture.c render.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/kdrive \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext

.include <bsd.lib.mk>
.include <bsd.x11.mk>
