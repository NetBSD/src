# $NetBSD: Makefile,v 1.9 2019/01/27 02:08:35 pgoyette Exp $

USE_SHLIBDIR=   yes

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
