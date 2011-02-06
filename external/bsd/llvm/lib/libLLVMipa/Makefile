#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:05 joerg Exp $

LIB=	LLVMipa

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis/IPA

SRCS+=	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	FindUsedTypes.cpp \
	GlobalsModRef.cpp \
	IPA.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
