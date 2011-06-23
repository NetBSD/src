#	$NetBSD: Makefile,v 1.2.4.1 2011/06/23 14:18:26 cherry Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.if defined(HAVE_GCC)
COPTS.amq_xdr.c+=	-fno-strict-aliasing
COPTS.amq.c+=		-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
