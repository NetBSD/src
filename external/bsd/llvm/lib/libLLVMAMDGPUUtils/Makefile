#	$NetBSD: Makefile,v 1.1 2019/03/10 12:14:06 mrg Exp $

LIB=	LLVMAMDGPUUtils

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMAMDGPUCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AMDGPU

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU/Utils

SRCS+=	AMDGPUBaseInfo.cpp \
	AMDKernelCodeTUtils.cpp \
	AMDGPUAsmUtils.cpp \
	AMDGPULaneDominator.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
