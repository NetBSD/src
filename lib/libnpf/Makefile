# $NetBSD: Makefile,v 1.1.6.3 2013/01/16 05:32:25 yamt Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=   yes

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
