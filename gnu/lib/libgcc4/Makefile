#	$NetBSD: Makefile,v 1.2 2008/09/23 06:59:42 skrll Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
