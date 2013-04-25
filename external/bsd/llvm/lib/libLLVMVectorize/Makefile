#	$NetBSD: Makefile,v 1.4 2013/04/25 13:59:57 joerg Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	BBVectorize.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	VecUtils.cpp \
	Vectorize.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
