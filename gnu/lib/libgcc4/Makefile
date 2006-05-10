#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

SUBDIR+=	libgcc

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

.include <bsd.subdir.mk>
