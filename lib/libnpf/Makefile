# $NetBSD: Makefile,v 1.10 2019/01/27 04:39:33 dholland Exp $

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
