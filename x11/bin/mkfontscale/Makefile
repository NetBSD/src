#	$NetBSD: Makefile,v 1.4 2004/03/06 03:09:36 rtr Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c hash.c ident.c

CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 \
		-DXFREE86_FT2

LDADD+=	-lfontenc -lfreetype -lz
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
