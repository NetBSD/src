# $NetBSD: Makefile,v 1.2 2005/02/20 00:28:20 christos Exp $

USE_SHLIBDIR=	yes
.include <bsd.own.mk>

WARNS=3
LINTFLAGS+= -Sw

LIB=	libradius
CPPFLAGS+=	-I.

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DWITH_SSL
.endif

SRCS=	radlib.c

MAN=	libradius.3 radius.conf.5

CLEANFILES+= libradius.cat3 radius.conf.cat5 radlib.d 

.include <bsd.lib.mk>
