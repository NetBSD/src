#	$NetBSD: Makefile,v 1.4 2014/03/17 09:02:16 mrg Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c hash.c ident.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lfontenc -lfreetype -lz -lbz2
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
