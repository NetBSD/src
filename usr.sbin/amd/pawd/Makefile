#	$NetBSD: Makefile,v 1.6 2008/08/29 00:02:24 gmcgarry Exp $

.include "${.CURDIR}/../Makefile.inc"

BINDIR=	/usr/bin

PROG=	pawd

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_clnt.c amq_xdr.c pawd.c
MAN+=	pawd.1

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.amq_xdr.c+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
