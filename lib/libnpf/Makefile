# $NetBSD: Makefile,v 1.7 2018/09/29 14:41:36 rmind Exp $

USE_SHLIBDIR=   yes

NOLINT=		# disabled deliberately

.include <bsd.own.mk>

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

WARNS=		5

.include <bsd.lib.mk>
