# $NetBSD: Makefile,v 1.8 2018/09/29 15:52:44 christos Exp $

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
