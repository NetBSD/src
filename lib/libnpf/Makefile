# $NetBSD: Makefile,v 1.5 2014/07/22 22:28:47 christos Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=   yes

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBDPLIBS+=	prop ${.CURDIR}/../libprop
LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.lib.mk>
