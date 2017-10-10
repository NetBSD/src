# $NetBSD: Makefile,v 1.13 2017/10/10 19:30:41 christos Exp $

.include <bsd.own.mk>

USE_FORT?=	yes	# network client

TCPDUMP=	${NETBSDSRCDIR}/external/bsd/tcpdump/dist
.PATH:		${TCPDUMP}

RUMPPRG=	ndp
SRCS=		ndp.c gmt2local.c
MAN=		ndp.8

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I. -I${TCPDUMP}

.if (${MKRUMP} != "no")
CPPFLAGS+= 	-DRUMP_ACTION
LDADD.rump+=	-lrumpres
DPADD.rump+=	${LIBRUMPRES}
.endif

.include <bsd.prog.mk>
