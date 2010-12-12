# $NetBSD: Makefile,v 1.8 2010/12/12 18:50:31 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

TCPDUMP= ${NETBSDSRCDIR}/external/bsd/tcpdump/dist
.PATH:	${TCPDUMP}

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN=	ndp.8

.if ${MKINET6} != "no"
CPPFLAGS+=-DINET6
.endif
CPPFLAGS+=-I. -I${TCPDUMP}

.include <bsd.prog.mk>
