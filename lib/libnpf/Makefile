# $NetBSD: Makefile,v 1.4.6.1 2014/08/10 06:52:04 tls Exp $

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
