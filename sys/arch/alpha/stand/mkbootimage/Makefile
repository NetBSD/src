# $NetBSD: Makefile,v 1.1 1999/04/02 08:40:26 cgd Exp $

.include <bsd.own.mk>

PROG=	mkbootimage
BINDIR=	/usr/mdec

WARNS?=	1

SRCS=	mkbootimage.c

MKMAN=	no

CFLAGS+= -I${.CURDIR}/../..
LDSTATIC?= -static

.include <bsd.prog.mk>
