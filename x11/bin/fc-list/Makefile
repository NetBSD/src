#	$NetBSD: Makefile,v 1.3 2004/03/06 03:09:36 rtr Exp $

.include <bsd.own.mk>

PROG=	fc-list

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.xc}/extras/fontconfig/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
