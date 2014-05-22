#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:14:44 yamt Exp $

LIB=	LLVMipa

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis/IPA

SRCS+=	CallPrinter.cpp \
	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	FindUsedTypes.cpp \
	GlobalsModRef.cpp \
	IPA.cpp \
	InlineCost.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
