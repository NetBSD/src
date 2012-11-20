# $NetBSD: Makefile,v 1.2.2.1 2012/11/20 03:00:44 tls Exp $

.include <bsd.own.mk>

LIB=		npf
MAN=		npf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBDPLIBS+=	prop ${.CURDIR}/../libprop
LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.lib.mk>
