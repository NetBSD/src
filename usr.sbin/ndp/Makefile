# $NetBSD: Makefile,v 1.3 2001/06/26 13:43:29 itojun Exp $

.PATH:	${.CURDIR}/../../dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${.CURDIR}/../../dist/tcpdump

.include <bsd.prog.mk>
