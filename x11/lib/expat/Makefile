#	$NetBSD: Makefile,v 1.4 2003/11/08 13:57:29 lukem Exp $

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
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	sed -e 's/@EXPAT_MAJOR_VERSION@/${EXPAT_MAJOR_VERSION}/' \
	    -e 's/@EXPAT_MINOR_VERSION@/${EXPAT_MINOR_VERSION}/' \
	    -e 's/@EXPAT_EDIT@/${EXPAT_MICRO_VERSION}/' ${.ALLSRC} > ${.TARGET}
CLEANFILES+=	expat.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
