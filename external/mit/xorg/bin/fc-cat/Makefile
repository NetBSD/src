#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:57 haad Exp $

.include <bsd.own.mk>

PROG=	fc-cat

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

FCARCH_DEPFILE=	fc-cat.c
.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
