# $NetBSD: Makefile,v 1.4 2002/09/18 03:54:34 lukem Exp $

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${NETBSDSRCDIR}/dist/tcpdump

.include <bsd.prog.mk>
