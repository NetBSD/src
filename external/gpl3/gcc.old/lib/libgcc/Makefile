#	$NetBSD: Makefile,v 1.1.1.1.4.2 2014/05/22 15:58:35 yamt Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
