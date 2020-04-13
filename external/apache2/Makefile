#	$NetBSD: Makefile,v 1.1.48.1 2020/04/13 07:45:46 martin Exp $

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
