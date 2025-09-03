#	$NetBSD: Makefile,v 1.1 2025/07/17 14:31:23 christos Exp $

.include <bsd.own.mk>

.if ${HAVE_OPENSSL} >= 35
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR:T}
.endif

.include <bsd.subdir.mk>
