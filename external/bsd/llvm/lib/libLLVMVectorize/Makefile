#	$NetBSD: Makefile,v 1.2.4.2 2013/06/23 06:28:23 tls Exp $

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
