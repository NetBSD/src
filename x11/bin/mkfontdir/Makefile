#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:07 lukem Exp $

.include <bsd.own.mk>

PROG=	mkfontdir

CPPFLAGS+=-DX_GZIP_FONT_COMPRESSION
CPPFLAGS+=-I${X11SRCDIR.xc}/include/fonts -I${X11SRCDIR.xc}/lib/font/include

LDADD+=	-lXfont -lfntstubs -lm -lz
DPADD+=	${LIBXFONT} ${LIBFNTSTUBS} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
