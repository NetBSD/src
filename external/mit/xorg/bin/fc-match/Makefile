#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:26 wrstuden Exp $

.include <bsd.own.mk>

PROG=	fc-match

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

FCARCH_DEPFILE=	fc-list.c
.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
