#	$NetBSD: Makefile,v 1.5.14.1 2018/05/21 04:35:53 pgoyette Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c hash.c ident.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

LDADD+=	-lfontenc -lfreetype -lz -lbz2
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
