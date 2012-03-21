# $NetBSD: Makefile,v 1.2 2012/03/21 05:37:42 matt Exp $

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
