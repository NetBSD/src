#	$NetBSD: Makefile,v 1.5.2.1 2004/04/29 05:46:38 jmc Exp $

.include <bsd.own.mk>

LIB=	expat
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}
.PATH:	${X11SRCDIR.xc}/extras/expat/lib

SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-DXML_BYTE_ORDER=0 -I${DIST}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
