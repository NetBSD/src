#	$NetBSD: Makefile,v 1.14.2.1 2017/03/20 06:56:00 pgoyette Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	BoundsChecking.cpp \
	DataFlowSanitizer.cpp \
	EfficiencySanitizer.cpp \
	GCOVProfiling.cpp \
	IndirectCallPromotion.cpp \
	InstrProfiling.cpp \
	Instrumentation.cpp \
	MemorySanitizer.cpp \
	PGOInstrumentation.cpp \
	SanitizerCoverage.cpp \
	ThreadSanitizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
