#	$NetBSD: Makefile,v 1.2.4.2 2008/10/19 22:39:28 haad Exp $

BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

PROG=	pawd

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_clnt.c amq_xdr.c pawd.c
MAN+=	pawd.1

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.if ${HAVE_GCC} == 4
COPTS.amq_xdr.c+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
