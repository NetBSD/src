#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	mkfontdir

CPPFLAGS+=-DX_GZIP_FONT_COMPRESSION
CPPFLAGS+=-I${X11SRCDIR.xc}/include/fonts -I${X11SRCDIR.xc}/lib/font/include

LDADD+=	-lXfont -lfntstubs -lm -lz
DPADD+=	${LIBXFONT} ${LIBFNTSTUBS} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
