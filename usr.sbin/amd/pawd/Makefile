#	$NetBSD: Makefile,v 1.4 2000/11/20 00:25:29 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

BINDIR=	/usr/bin

PROG=	pawd

DIST=	${IDIST}/amq
.PATH:	${DIST}

SRCS=	amq_clnt.c amq_xdr.c pawd.c
MAN+=	pawd.1

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU}

.include <bsd.prog.mk>
