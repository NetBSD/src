#	$NetBSD: Makefile,v 1.7.2.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

LIB=		expat

DIST=	${X11SRCDIR.xc}/lib/${LIB}

.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib
SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-DXML_BYTE_ORDER=0 -I${DIST}
COPTS.xmlparse.c=	-Wno-error # macppc
COPTS.xmlrole.c=	-Wno-error # macppc
COPTS.xmltok.c=		-Wno-error # macppc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
