#	$NetBSD: Makefile,v 1.6.2.1 2012/11/20 02:58:02 tls Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	BlackList.cpp \
	BoundsChecking.cpp \
	EdgeProfiling.cpp \
	GCOVProfiling.cpp \
	Instrumentation.cpp \
	OptimalEdgeProfiling.cpp \
	PathProfiling.cpp \
	ProfilingUtils.cpp \
	ThreadSanitizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
