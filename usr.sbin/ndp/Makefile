# $NetBSD: Makefile,v 1.1 1999/07/01 20:33:56 itojun Exp $

.PATH:	${.CURDIR}/../tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CFLAGS+=-DINET6
CFLAGS+=-I. -I${.CURDIR}/../tcpdump

.include <bsd.prog.mk>
