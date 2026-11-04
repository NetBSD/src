# $NetBSD: Makefile,v 1.6.14.1 2026/11/04 23:19:29 perseant Exp $

.include <bsd.own.mk>

LIB=		nvmm
MAN=		libnvmm.3

SRCS=		libnvmm.c

INCS=		nvmm.h
INCSDIR=	/usr/include

WARNS=		5

COPTS.libnvmm.c+=	${CC_WNO_DANGLING_POINTER}

.include <bsd.lib.mk>
