#	$NetBSD: Makefile,v 1.1.1.1.8.2 2014/08/19 23:58:06 tls Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
