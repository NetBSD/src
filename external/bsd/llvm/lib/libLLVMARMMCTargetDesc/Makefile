#	$NetBSD: Makefile,v 1.8 2014/01/15 22:21:34 joerg Exp $

LIB=	LLVMARMMCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/MCTargetDesc

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

SRCS+=	ARMAsmBackend.cpp \
	ARMBuildAttrs.cpp \
	ARMELFObjectWriter.cpp \
	ARMELFStreamer.cpp \
	ARMMCAsmInfo.cpp \
	ARMMCCodeEmitter.cpp \
	ARMMCExpr.cpp \
	ARMMCTargetDesc.cpp \
	ARMMachORelocationInfo.cpp \
	ARMMachObjectWriter.cpp \
	ARMUnwindOpAsm.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
