# $NetBSD: Makefile,v 1.9 2005/11/26 16:09:30 christos Exp $

USE_SHLIBDIR=	yes
.include <bsd.own.mk>

WARNS=3
LINTFLAGS+= -Sw

LIB=	radius
CPPFLAGS+=	-I.

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DWITH_SSL
LIBDPLIBS=crypto ${.CURDIR}/../libcrypto
.endif

SRCS=	radlib.c
INCS=	radlib.h radlib_vs.h
INCSDIR=/usr/include

MAN=	libradius.3 radius.conf.5

CLEANFILES+= libradius.cat3 radius.conf.cat5 radlib.d 

.include <bsd.lib.mk>
