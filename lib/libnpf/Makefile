# $NetBSD: Makefile,v 1.6 2016/01/05 13:07:47 christos Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=   yes

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBDPLIBS+=	prop ${.CURDIR}/../libprop

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.lib.mk>
