#	$NetBSD: Makefile,v 1.2 2013/08/22 00:16:54 matt Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
