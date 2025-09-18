# $NetBSD: Makefile,v 1.7 2025/09/18 03:36:02 mrg Exp $

.include <bsd.own.mk>

LIB=		nvmm
MAN=		libnvmm.3

SRCS=		libnvmm.c

INCS=		nvmm.h
INCSDIR=	/usr/include

WARNS=		5

COPTS.libnvmm.c+=	${CC_WNO_DANGLING_POINTER}

.include <bsd.lib.mk>
