#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:09:09 tls Exp $

LIB=	LLVMARMMCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/MCTargetDesc

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

SRCS+=	ARMAsmBackend.cpp \
	ARMELFObjectWriter.cpp \
	ARMELFStreamer.cpp \
	ARMMCAsmInfo.cpp \
	ARMMCCodeEmitter.cpp \
	ARMMCExpr.cpp \
	ARMMCTargetDesc.cpp \
	ARMTargetStreamer.cpp \
	ARMMachORelocationInfo.cpp \
	ARMMachObjectWriter.cpp \
	ARMUnwindOpAsm.cpp \
	ARMWinCOFFObjectWriter.cpp \
	ARMWinCOFFStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
