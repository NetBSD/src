#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:34 wrstuden Exp $

.include <bsd.own.mk>

LIB=		expat

DIST=	${X11SRCDIR.${LIB}}/lib

.PATH:	${DIST}
.PATH:	${X11SRCDIR.${LIB}}/lib
SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h expat_external.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-DXML_BYTE_ORDER=0 -I${DIST}
CPPFLAGS+=	-DHAVE_MEMMOVE=1
COPTS.xmlparse.c=	-Wno-error # macppc
COPTS.xmlrole.c=	-Wno-error # macppc
COPTS.xmltok.c=		-Wno-error # macppc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
