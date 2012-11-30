# $NetBSD: Makefile,v 1.1.8.1 2012/11/30 04:27:54 msaitoh Exp $

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

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.lib.mk>
