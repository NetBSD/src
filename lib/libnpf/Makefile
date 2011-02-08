# $NetBSD: Makefile,v 1.1.2.2 2011/02/08 19:01:36 bouyer Exp $

.include <bsd.own.mk>

LIB=		npf
MAN=		npf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBDPLIBS+=	prop ${.CURDIR}/../libprop
LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.lib.mk>
