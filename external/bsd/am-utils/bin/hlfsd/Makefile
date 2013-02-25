#	$NetBSD: Makefile,v 1.3.8.1 2013/02/25 00:24:53 tls Exp $

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
