#	$NetBSD: Makefile,v 1.3.10.1 2015/06/04 20:05:45 snj Exp $

LIB=	LLVMipa

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis/IPA

SRCS+=	CallPrinter.cpp \
	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	GlobalsModRef.cpp \
	IPA.cpp \
	InlineCost.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
