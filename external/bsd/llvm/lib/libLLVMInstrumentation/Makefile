#	$NetBSD: Makefile,v 1.2.4.3 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	BoundsChecking.cpp \
	DataFlowSanitizer.cpp \
	DebugIR.cpp \
	GCOVProfiling.cpp \
	Instrumentation.cpp \
	MemorySanitizer.cpp \
	ThreadSanitizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
