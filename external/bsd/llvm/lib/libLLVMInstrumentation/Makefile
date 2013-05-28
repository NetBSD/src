#	$NetBSD: Makefile,v 1.9 2013/05/28 17:37:34 joerg Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	BlackList.cpp \
	BoundsChecking.cpp \
	DebugIR.cpp \
	EdgeProfiling.cpp \
	GCOVProfiling.cpp \
	Instrumentation.cpp \
	MemorySanitizer.cpp \
	OptimalEdgeProfiling.cpp \
	PathProfiling.cpp \
	ProfilingUtils.cpp \
	ThreadSanitizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
