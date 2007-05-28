# $NetBSD: Makefile,v 1.6 2007/05/28 12:06:38 tls Exp $

WARNS?=4
.include <bsd.own.mk>

USE_FORT?= yes	# network client

.PATH:	${NETBSDSRCDIR}/dist/tcpdump

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${NETBSDSRCDIR}/dist/tcpdump

.include <bsd.prog.mk>
