#	$NetBSD: Makefile,v 1.3.24.1 2018/07/28 04:37:21 pgoyette Exp $

.include <bsd.own.mk>

PROG=	fc-match

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.
CPPFLAGS+=	-DHAVE_GETOPT_LONG=1

LDADD+=	-lfontconfig -lfreetype -lexpat -lbz2 -lz
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBBZ2} ${LIBZ}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
