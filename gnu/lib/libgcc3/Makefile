#	$NetBSD: Makefile,v 1.23 2004/02/25 21:53:49 matt Exp $

SUBDIR+=	libgcc libgcc_eh

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_s
.endif

.include <bsd.subdir.mk>
