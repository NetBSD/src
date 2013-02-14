#	$NetBSD: Makefile,v 1.4 2013/02/14 01:57:43 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	hlfsd

DIST=	${IDIST}/hlfsd
.PATH:	${DIST}

SRCS=	hlfsd.c homedir.c nfs_prot_svc.c stubs.c
MAN+=	hlfsd.8

CPPFLAGS+=	-I${DIST}
LDADD+=		-lrpcsvc
DPADD+=		${LIBRPCSVC}

.include <bsd.prog.mk>
