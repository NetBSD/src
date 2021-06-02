#	$NetBSD: Makefile,v 1.3.8.1 2021/06/02 14:46:18 cjep Exp $

.include <bsd.own.mk>

PROG=	fc-cat

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.
CPPFLAGS+=	-DHAVE_GETOPT_LONG=1

LDADD+=	${XLIBLDADD_FC}
DPADD+=	${XLIBDPADD_FC}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
