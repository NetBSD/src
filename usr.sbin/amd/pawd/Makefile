#	$NetBSD: Makefile,v 1.5 2006/05/11 09:08:09 mrg Exp $

.include "${.CURDIR}/../Makefile.inc"

BINDIR=	/usr/bin

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
