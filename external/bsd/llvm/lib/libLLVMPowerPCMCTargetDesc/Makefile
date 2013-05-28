#	$NetBSD: Makefile,v 1.4 2013/05/28 17:37:35 joerg Exp $

LIB=	LLVMPowerPCMCTargetDesc

.include <bsd.init.mk>

PPC_OBJDIR!=	cd ${.CURDIR}/../libLLVMPowerPCCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${PPC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/MCTargetDesc

SRCS+=	PPCAsmBackend.cpp \
	PPCMCAsmInfo.cpp \
	PPCELFObjectWriter.cpp \
	PPCMCCodeEmitter.cpp \
	PPCMCExpr.cpp \
	PPCMCTargetDesc.cpp \
	PPCPredicates.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
