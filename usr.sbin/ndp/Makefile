# $NetBSD: Makefile,v 1.2 1999/07/03 06:26:25 itojun Exp $

.PATH:	${.CURDIR}/../tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${.CURDIR}/../tcpdump

.include <bsd.prog.mk>
