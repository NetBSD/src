#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:03 lukem Exp $

.include <bsd.own.mk>

PROG=	fc-cache

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.xc}/lib/fontconfig/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
