#	$NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:39:28 haad Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	hlfsd

DIST=	${IDIST}/hlfsd
.PATH:	${DIST}

SRCS=	hlfsd.c homedir.c nfs_prot_svc.c stubs.c
MAN+=	hlfsd.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU} -lrpcsvc

.include <bsd.prog.mk>
