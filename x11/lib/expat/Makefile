#	$NetBSD: Makefile,v 1.5 2004/03/06 03:05:39 rtr Exp $

.include <bsd.own.mk>

LIB=	expat
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib

SRCS=	xmlparse.c xmlrole.c xmltok.c

.include "Makefile.expatdefs"

CPPFLAGS+=	${EXPAT_FLAGS} -I${DIST}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
