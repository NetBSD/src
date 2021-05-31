#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:06 cjep Exp $

LIB=	LLVMAMDGPUUtils

.include <bsd.init.mk>

AMDGPU_OBJDIR!=	cd ${.CURDIR}/../libLLVMAMDGPUCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${AMDGPU_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AMDGPU

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU/Utils

SRCS+=	AMDGPUAsmUtils.cpp \
	AMDGPUBaseInfo.cpp \
	AMDGPULDSUtils.cpp \
	AMDGPUPALMetadata.cpp \
	AMDKernelCodeTUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
