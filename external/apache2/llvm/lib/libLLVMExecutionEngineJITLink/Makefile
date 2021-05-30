#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMExecutionEngineJITLink

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JITLink

SRCS+=	EHFrameSupport.cpp \
	ELF.cpp \
	ELF_x86_64.cpp \
	JITLink.cpp \
	JITLinkGeneric.cpp \
	JITLinkMemoryManager.cpp \
	MachO_arm64.cpp \
	MachO.cpp \
	MachOLinkGraphBuilder.cpp \
	MachO_x86_64.cpp \
	x86_64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
