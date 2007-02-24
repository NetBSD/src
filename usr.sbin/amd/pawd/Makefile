#	$NetBSD: Makefile,v 1.4.8.1 2007/02/24 12:17:35 bouyer Exp $

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
