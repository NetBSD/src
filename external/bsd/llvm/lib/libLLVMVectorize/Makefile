#	$NetBSD: Makefile,v 1.2.2.4 2014/05/22 16:14:44 yamt Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	BBVectorize.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	Vectorize.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
