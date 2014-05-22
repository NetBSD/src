#	$NetBSD: Makefile,v 1.4.2.1 2014/05/22 15:41:06 yamt Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}

.include <bsd.prog.mk>
