#	$NetBSD: Makefile,v 1.1 2011/07/17 20:07:58 joerg Exp $

LIB=	LLVMPowerPCMCTargetDesc

.include <bsd.init.mk>

PPC_OBJDIR!=	cd ${.CURDIR}/../libLLVMPowerPCCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${PPC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/MCTargetDesc

SRCS+=	PPCMCAsmInfo.cpp \
	PPCMCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
