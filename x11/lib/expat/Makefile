#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:53 lukem Exp $

.include <bsd.own.mk>

LIB=	expat
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib

SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h
INCSDIR=${X11INCDIR}

.include "Makefile.expatdefs"

CPPFLAGS+=	${EXPAT_FLAGS} -I${DIST}

expat.h: expat.h.in
	rm -f ${.TARGET}
	sed -e 's/@EXPAT_MAJOR_VERSION@/${EXPAT_MAJOR_VERSION}/' \
	    -e 's/@EXPAT_MINOR_VERSION@/${EXPAT_MINOR_VERSION}/' \
	    -e 's/@EXPAT_EDIT@/${EXPAT_MICRO_VERSION}/' ${.ALLSRC} > ${.TARGET}
CLEANFILES+=	expat.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
