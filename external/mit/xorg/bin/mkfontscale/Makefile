#	$NetBSD: Makefile,v 1.6.2.1 2019/06/10 22:02:35 christos Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c constlist.c hash.c ident.c list.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

LDADD+=	-lfontenc -lfreetype -lz -lbz2
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
