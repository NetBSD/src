#	$NetBSD: Makefile,v 1.2.4.1 2012/11/20 02:58:02 tls Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	BBVectorize.cpp \
	LoopVectorize.cpp \
	Vectorize.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
