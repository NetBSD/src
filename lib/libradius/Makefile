# $NetBSD: Makefile,v 1.15 2018/02/07 22:51:31 christos Exp $

USE_FORT?= yes	# network protocol library

USE_SHLIBDIR=	yes
.include <bsd.own.mk>

WARNS=3
LINTFLAGS+= -Sw

LIB=	radius
CPPFLAGS+=	-I.
CPPFLAGS+=	-DOPENSSL_API_COMPAT=0x10100000L

CPPFLAGS+=	-DWITH_SSL
LIBDPLIBS+=crypto ${.CURDIR}/../../crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto

SRCS=	radlib.c
INCS=	radlib.h radlib_vs.h
INCSDIR=/usr/include

MAN=	libradius.3 radius.conf.5

CLEANFILES+= libradius.cat3 radius.conf.cat5 radlib.d 

.include <bsd.lib.mk>
