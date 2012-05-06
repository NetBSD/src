# $NetBSD: Makefile,v 1.1.2.1 2012/05/06 18:07:01 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= mj
SRCS= mj.c
MAN= libmj.3
WARNS=4

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/libmj

INCS+= mj.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
