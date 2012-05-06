# $NetBSD: Makefile,v 1.1.2.1 2012/05/06 18:07:02 agc Exp $

LIB=netpgpverify
SRCS= verify.c
CPPFLAGS+=-I${EXTDIST}/src/libbn -I${EXTDIST}/src/librsa
MAN=libnetpgpverify.3
WARNS=4

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/libverify

INCS=verify.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>
