#	$NetBSD: Makefile,v 1.3 2012/01/11 23:24:48 joerg Exp $

LIB=	LLVMPowerPCMCTargetDesc

.include <bsd.init.mk>

PPC_OBJDIR!=	cd ${.CURDIR}/../libLLVMPowerPCCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${PPC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/MCTargetDesc

SRCS+=	PPCAsmBackend.cpp \
	PPCMCAsmInfo.cpp \
	PPCELFObjectWriter.cpp \
	PPCMCCodeEmitter.cpp \
	PPCMCTargetDesc.cpp \
	PPCPredicates.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
