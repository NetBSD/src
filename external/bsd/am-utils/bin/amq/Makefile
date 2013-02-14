#	$NetBSD: Makefile,v 1.5 2013/02/14 01:57:43 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}

.include <bsd.prog.mk>
