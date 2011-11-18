#	$NetBSD: Makefile,v 1.3 2011/11/18 15:56:54 joerg Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	EdgeProfiling.cpp \
	GCOVProfiling.cpp \
	Instrumentation.cpp \
	OptimalEdgeProfiling.cpp \
	PathProfiling.cpp \
	ProfilingUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
