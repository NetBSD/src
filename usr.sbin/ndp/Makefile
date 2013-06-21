# $NetBSD: Makefile,v 1.9 2013/06/21 12:50:50 uwe Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

TCPDUMP= ${NETBSDSRCDIR}/external/bsd/tcpdump/dist
.PATH:	${TCPDUMP}

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-I. -I${TCPDUMP}

.include <bsd.prog.mk>
