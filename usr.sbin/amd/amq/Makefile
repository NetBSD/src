#	$NetBSD: Makefile,v 1.13.8.1 2007/02/24 12:17:33 bouyer Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.include <bsd.prog.mk>
