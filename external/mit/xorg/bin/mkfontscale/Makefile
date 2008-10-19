#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c hash.c ident.c

CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 \
		-DXFREE86_FT2

LDADD+=	-lfontenc -lfreetype -lz
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
