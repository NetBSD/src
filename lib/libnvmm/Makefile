# $NetBSD: Makefile,v 1.4 2018/11/13 09:14:14 martin Exp $

.include <bsd.own.mk>

.if ${MACHINE_ARCH} == "x86_64" && ${MLIBDIR:Unone} != "i386"

LIB=		nvmm
MAN=		libnvmm.3

SRCS=		libnvmm.c libnvmm_x86.c

INCS=		nvmm.h
INCSDIR=	/usr/include

WARNS=		5

.else
LIB=
LIBDPLIBS=
.endif

.include <bsd.lib.mk>

