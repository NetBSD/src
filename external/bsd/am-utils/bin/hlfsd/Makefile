#	$NetBSD: Makefile,v 1.3.2.1 2014/05/22 15:41:06 yamt Exp $

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
