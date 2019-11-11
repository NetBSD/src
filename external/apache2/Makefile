#	$NetBSD: Makefile,v 1.3 2019/11/11 22:44:57 joerg Exp $

.include <bsd.own.mk>

.if (defined(MKARGON2) && ${MKARGON2} != "no")
SUBDIR+= argon2
.endif

.if (${MKLLVM} != "no" || ${MKLLVMRT} != "no")
SUBDIR+= llvm
.endif

.if (${MKMDNS} != "no")
SUBDIR+= mDNSResponder
.endif

.include <bsd.subdir.mk>
