# $NetBSD: Makefile,v 1.5 2005/02/20 00:36:47 christos Exp $

USE_SHLIBDIR=	yes
.include <bsd.own.mk>

WARNS=3
LINTFLAGS+= -Sw

LIB=	radius
CPPFLAGS+=	-I.

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DWITH_SSL
.endif

SRCS=	radlib.c
INCS=	radlib.h radlib_vs.h
INCSDIR=/usr/include

MAN=	libradius.3 radius.conf.5

CLEANFILES+= libradius.cat3 radius.conf.cat5 radlib.d 

.include <bsd.lib.mk>
