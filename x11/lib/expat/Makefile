#	$NetBSD: Makefile,v 1.6 2004/04/04 00:20:45 lukem Exp $

.include <bsd.own.mk>

LIB=	expat
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib

SRCS=	xmlparse.c xmlrole.c xmltok.c

.include "Makefile.expatdefs"

CPPFLAGS+=	${EXPAT_FLAGS} -I${DIST} -I${X11SRCDIR.xc}/extras/expat/lib

.include <bsd.x11.mk>
.include <bsd.lib.mk>
