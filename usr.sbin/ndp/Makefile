# $NetBSD: Makefile,v 1.7 2009/04/22 15:23:06 lukem Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

.PATH:	${NETBSDSRCDIR}/dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${NETBSDSRCDIR}/dist/tcpdump

.include <bsd.prog.mk>
