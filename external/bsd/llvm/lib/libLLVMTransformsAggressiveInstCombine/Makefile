#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 21:50:55 christos Exp $

LIB=	LLVMTransformsAggressiveInstCombine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/AggressiveInstCombine

SRCS+=	AggressiveInstCombine.cpp \
	TruncInstCombine.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
