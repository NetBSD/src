# $NetBSD: Makefile,v 1.1.6.1 2012/10/30 18:46:19 yamt Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= mj
SRCS= mj.c
MAN= libmj.3

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/libmj

INCS+= mj.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
