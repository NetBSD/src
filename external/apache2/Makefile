#	$NetBSD: Makefile,v 1.2 2019/10/09 13:13:08 jhigh Exp $

.include <bsd.own.mk>

.if (${MKMDNS} != "no")
SUBDIR+= mDNSResponder
.endif

.if (defined(MKARGON2) && ${MKARGON2} != "no")
SUBDIR+= argon2
.endif

.include <bsd.subdir.mk>
