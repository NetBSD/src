# $NetBSD: Makefile,v 1.1.1.1 2005/02/19 23:56:30 manu Exp $

USE_SHLIBDIR=	yes
NOLINT=         # To be fixed later

.include <bsd.own.mk>

LIB=	libradius
CFLAGS+=-g
CPPFLAGS+=	-I.

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DWITH_SSL
.endif

SRCS=	radlib.c

MAN=	libradius.3 radius.conf.5

CLEANFILES+= libradius.cat3 radius.conf.cat5 radlib.d 

.include <bsd.lib.mk>
