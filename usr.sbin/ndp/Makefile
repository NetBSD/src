# $NetBSD: Makefile,v 1.8.12.1 2013/06/23 06:29:05 tls Exp $

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
