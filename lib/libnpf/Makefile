# $NetBSD: Makefile,v 1.1.6.1 2012/04/17 00:05:29 yamt Exp $

.include <bsd.own.mk>

LIB=		npf
MAN=		npf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBDPLIBS+=	prop ${.CURDIR}/../libprop
LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS?=		5
NOLINT=		# defined (note: deliberately)

.include <bsd.lib.mk>
