#	$NetBSD: Makefile,v 1.2 2009/06/10 01:56:46 mrg Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c hash.c ident.c

CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 \
		-DXFREE86_FT2 -DX_BZIP2_FONT_COMPRESSION

LDADD+=	-lfontenc -lfreetype -lz -lbz2
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
