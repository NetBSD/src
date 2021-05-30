#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:54 joerg Exp $

LIB=	LLVMPowerPCMCTargetDesc

.include <bsd.init.mk>

PPC_OBJDIR!=	cd ${.CURDIR}/../libLLVMPowerPCCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${PPC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/MCTargetDesc

SRCS+=	PPCAsmBackend.cpp \
	PPCELFObjectWriter.cpp \
	PPCELFStreamer.cpp \
	PPCInstPrinter.cpp \
	PPCMCAsmInfo.cpp \
	PPCMCCodeEmitter.cpp \
	PPCMCExpr.cpp \
	PPCMCTargetDesc.cpp \
	PPCPredicates.cpp \
	PPCXCOFFObjectWriter.cpp \
	PPCXCOFFStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
