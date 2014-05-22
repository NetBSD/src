#	$NetBSD: Makefile,v 1.1.2.1 2014/05/22 16:53:16 yamt Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
