#	$NetBSD: Makefile,v 1.3.4.1 2011/06/23 14:18:27 cherry Exp $

BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

PROG=	pawd

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_clnt.c amq_xdr.c pawd.c
MAN+=	pawd.1

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.if defined(HAVE_GCC)
COPTS.amq_xdr.c+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
