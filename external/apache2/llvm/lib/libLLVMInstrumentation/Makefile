#	$NetBSD: Makefile,v 1.4 2023/06/03 08:52:53 lukem Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	BoundsChecking.cpp \
	CGProfile.cpp \
	ControlHeightReduction.cpp \
	DataFlowSanitizer.cpp \
	GCOVProfiling.cpp \
	HWAddressSanitizer.cpp \
	IndirectCallPromotion.cpp \
	InstrOrderFile.cpp \
	InstrProfiling.cpp \
	Instrumentation.cpp \
	MemorySanitizer.cpp \
	MemProfiler.cpp \
	PGOInstrumentation.cpp \
	PGOMemOPSizeOpt.cpp \
	PoisonChecking.cpp \
	SanitizerCoverage.cpp \
	ThreadSanitizer.cpp \
	ValueProfileCollector.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
