#	$NetBSD: Makefile,v 1.2 2011/06/22 02:49:42 mrg Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	hlfsd

DIST=	${IDIST}/hlfsd
.PATH:	${DIST}

SRCS=	hlfsd.c homedir.c nfs_prot_svc.c stubs.c
MAN+=	hlfsd.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU} -lrpcsvc

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.stubs.c+=	-fno-strict-aliasing
.endif
