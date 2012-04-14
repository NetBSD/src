#	$NetBSD: Makefile,v 1.8 2012/04/14 15:11:18 joerg Exp $

LIB=	LLVMTarget

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target

SRCS+=	Mangler.cpp \
	Target.cpp \
	TargetData.cpp \
	TargetELFWriterInfo.cpp \
	TargetInstrInfo.cpp \
	TargetIntrinsicInfo.cpp \
	TargetJITInfo.cpp \
	TargetLibraryInfo.cpp \
	TargetLoweringObjectFile.cpp \
	TargetMachine.cpp \
	TargetMachineC.cpp \
	TargetRegisterInfo.cpp \
	TargetSubtargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
