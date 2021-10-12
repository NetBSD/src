#	$NetBSD: Makefile,v 1.4 2021/10/12 17:24:36 nia Exp $

.include <bsd.own.mk>

.if (${MKLLVM} != "no" || ${MKLLVMRT} != "no")
SUBDIR+= llvm
.endif

.if (${MKMDNS} != "no")
SUBDIR+= mDNSResponder
.endif

.include <bsd.subdir.mk>
