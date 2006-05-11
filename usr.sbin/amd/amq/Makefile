#	$NetBSD: Makefile,v 1.14 2006/05/11 09:08:09 mrg Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amq

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_xdr.c amq_clnt.c amq.c
MAN+=	amq.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.if ${HAVE_GCC} == 4
COPTS.amq_xdr.c+=	-fno-strict-aliasing
COPTS.amq.c+=		-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
