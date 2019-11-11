#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:13 joerg Exp $

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
