# $NetBSD: Makefile,v 1.6.16.1 2019/06/10 22:05:25 christos Exp $

USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

WARNS=		5

.include <bsd.lib.mk>
