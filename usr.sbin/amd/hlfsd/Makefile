#	$NetBSD: Makefile,v 1.1.1.2 2000/11/20 00:16:18 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	hlfsd

DIST=	${IDIST}/hlfsd
.PATH:	${DIST}

SRCS=	hlfsd.c homedir.c nfs_prot_svc.c stubs.c
MAN+=	hlfsd.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU} -lrpcsvc

.include <bsd.prog.mk>
