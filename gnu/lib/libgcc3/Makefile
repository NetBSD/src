#	$NetBSD: Makefile,v 1.25 2004/02/27 14:52:01 skrll Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

.include <bsd.subdir.mk>
