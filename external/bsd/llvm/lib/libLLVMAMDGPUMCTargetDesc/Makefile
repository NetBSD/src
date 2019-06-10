#	$NetBSD: Makefile,v 1.1.2.2 2019/06/10 21:50:50 christos Exp $

LIB=	LLVMAMDGPUMCTargetDesc

.include <bsd.init.mk>

AMDGPU_OBJDIR!=	cd ${.CURDIR}/../libLLVMAMDGPUCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${AMDGPU_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU/MCTargetDesc

SRCS+=	AMDGPUAsmBackend.cpp \
	AMDGPUELFObjectWriter.cpp \
	AMDGPUELFStreamer.cpp \
	AMDGPUMCAsmInfo.cpp \
	AMDGPUMCCodeEmitter.cpp \
	AMDGPUMCTargetDesc.cpp \
	AMDGPUTargetStreamer.cpp \
	R600MCCodeEmitter.cpp \
	R600MCTargetDesc.cpp \
	SIMCCodeEmitter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
