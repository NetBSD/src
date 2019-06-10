#	$NetBSD: Makefile,v 1.1.2.2 2019/06/10 21:50:50 christos Exp $

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
