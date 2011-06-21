#	$NetBSD: Makefile,v 1.1.1.1 2011/06/21 02:48:27 mrg Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
