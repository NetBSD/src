#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

.include <bsd.own.mk>

LIB=	expat
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib

SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h
INCSDIR=${X11INCDIR}

MAJOR_VERSION=	1
MINOR_VERSION=	95
MICRO_VERSION=	2

CPPFLAGS+=	-DVERSION=\"${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}\"
CPPFLAGS+=	-DXML_BYTE_ORDER=0
CPPFLAGS+=	-I${DIST}

expat.h: expat.h.in
	rm -f ${.TARGET}
	sed -e 's/@EXPAT_MAJOR_VERSION@/${MAJOR_VERSION}/' \
	    -e 's/@EXPAT_MINOR_VERSION@/${MINOR_VERSION}/' \
	    -e 's/@EXPAT_EDIT@/${MICRO_VERSION}/' ${.ALLSRC} > ${.TARGET}
CLEANFILES+=	expat.h

.include <bsd.lib.mk>
.include <bsd.x11.mk>
