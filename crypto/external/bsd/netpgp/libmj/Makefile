# $NetBSD: Makefile,v 1.3 2025/09/18 03:35:59 mrg Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= mj
SRCS= mj.c
MAN= libmj.3

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/libmj

INCS+= mj.h
INCSDIR=/usr/include

COPTS.mj.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.lib.mk>
