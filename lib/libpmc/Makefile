#	$NetBSD: Makefile,v 1.1 2002/08/09 00:03:15 thorpej Exp $

.include <bsd.own.mk>

MAN=	pmc.3

.if exists(${.CURDIR}/pmc_${MACHINE_CPU}.c)
LIB=	pmc
SRCS=	pmc.c
SRCS+=	pmc_${MACHINE_CPU}.c

INCS=	pmc.h
INCSDIR=/usr/include
.include <bsd.lib.mk>
.else
.include <bsd.man.mk>
.endif
