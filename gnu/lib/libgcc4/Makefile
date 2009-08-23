#	$NetBSD: Makefile,v 1.2.10.1 2009/08/23 20:24:06 matt Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

SUBDIR+=	libgcc_eh

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
