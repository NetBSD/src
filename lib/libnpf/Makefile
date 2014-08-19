# $NetBSD: Makefile,v 1.2.2.3 2014/08/20 00:02:18 tls Exp $

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
