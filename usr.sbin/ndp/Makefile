# $NetBSD: Makefile,v 1.6.20.1 2009/05/13 19:20:31 jym Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

.PATH:	${NETBSDSRCDIR}/dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${NETBSDSRCDIR}/dist/tcpdump

.include <bsd.prog.mk>
