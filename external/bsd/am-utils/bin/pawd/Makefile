#	$NetBSD: Makefile,v 1.4.2.1 2014/05/22 15:41:06 yamt Exp $

BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

PROG=	pawd

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_clnt.c amq_xdr.c pawd.c
MAN+=	pawd.1

CPPFLAGS+=	-I${DIST}

.if defined(HAVE_GCC)
COPTS.amq_xdr.c+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
