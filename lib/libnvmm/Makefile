# $NetBSD: Makefile,v 1.3 2018/11/13 09:00:09 martin Exp $

.if ${MACHINE_ARCH} == "x86_64" && ${MLIBDIR:Unone} != "i386"

.include <bsd.own.mk>

LIB=		nvmm
MAN=		libnvmm.3

SRCS=		libnvmm.c libnvmm_x86.c

INCS=		nvmm.h
INCSDIR=	/usr/include

WARNS=		5

.include <bsd.lib.mk>

.endif
