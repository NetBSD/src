#	$NetBSD: Makefile,v 1.4.8.1 2013/02/25 00:24:53 tls Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}

.include <bsd.prog.mk>
