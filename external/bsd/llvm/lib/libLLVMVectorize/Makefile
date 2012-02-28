#	$NetBSD: Makefile,v 1.1 2012/02/28 17:09:31 joerg Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	BBVectorize.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
