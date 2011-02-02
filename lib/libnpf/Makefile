# $NetBSD: Makefile,v 1.1 2011/02/02 02:20:25 rmind Exp $

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
