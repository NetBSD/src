#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:06 joerg Exp $

LIB=	LLVMExecutionEngineJITLink

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JITLink

SRCS+=	EHFrameSupport.cpp \
	JITLink.cpp \
	JITLinkGeneric.cpp \
	JITLinkMemoryManager.cpp \
	MachO_arm64.cpp \
	MachOAtomGraphBuilder.cpp \
	MachO.cpp \
	MachOLinkGraphBuilder.cpp \
	MachO_x86_64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
