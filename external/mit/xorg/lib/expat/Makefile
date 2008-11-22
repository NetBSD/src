#	$NetBSD: Makefile,v 1.2 2008/11/22 00:57:23 cube Exp $

.include <bsd.own.mk>

LIB=		expat

DIST=	${X11SRCDIR.${LIB}}/lib

.PATH:	${DIST}
.PATH:	${X11SRCDIR.${LIB}}/lib
SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=	expat.h expat_external.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-I${DIST} -I${.CURDIR}
CPPFLAGS+=	-DHAVE_EXPAT_CONFIG_H
COPTS.xmlparse.c=	-Wno-error # macppc
COPTS.xmlrole.c=	-Wno-error # macppc
COPTS.xmltok.c=		-Wno-error # macppc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
