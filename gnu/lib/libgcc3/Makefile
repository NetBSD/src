#	$NetBSD: Makefile,v 1.22 2004/02/15 19:52:27 skrll Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

.include <bsd.subdir.mk>
