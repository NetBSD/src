# $NetBSD: Makefile,v 1.12 2019/08/27 14:54:17 martin Exp $

USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBNV_DIST=	${NETBSDSRCDIR}/external/bsd/libnv/lib

.include "${LIBNV_DIST}/srcs.mk"
SRCS+=		${LIBNV_SRCS}
CPPFLAGS.npf.c	+=	-I${NVSRC_DISTPATH}

WARNS=		5

.include <bsd.lib.mk>
