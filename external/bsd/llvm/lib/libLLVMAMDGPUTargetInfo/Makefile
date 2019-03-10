#	$NetBSD: Makefile,v 1.1 2019/03/10 12:14:06 mrg Exp $

LIB=	LLVMAMDGPUTargetInfo

.include <bsd.init.mk>

AMDGPU_OBJDIR!=	cd ${.CURDIR}/../libLLVMAMDGPUCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${AMDGPU_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU/TargetInfo

SRCS+=	AMDGPUTargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
