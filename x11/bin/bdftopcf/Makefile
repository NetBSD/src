#	$NetBSD: Makefile,v 1.3 2004/03/06 03:09:36 rtr Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/font/bitmap

LDADD+=	-lXfont -lfntstubs -lfreetype -lm -lz
DPADD+=	${LIBXFONT} ${LIBFNTSTUBS} ${LIBFREETYPE} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
