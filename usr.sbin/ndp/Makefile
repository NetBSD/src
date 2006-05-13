# $NetBSD: Makefile,v 1.5 2006/05/13 20:46:15 christos Exp $

WARNS?=4
.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${NETBSDSRCDIR}/dist/tcpdump

.include <bsd.prog.mk>
