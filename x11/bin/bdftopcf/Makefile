#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:01 lukem Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/font/bitmap

LDADD+=	-lXfont -lfntstubs -lm -lz
DPADD+=	${LIBXFONT} ${LIBFNTSTUBS} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
