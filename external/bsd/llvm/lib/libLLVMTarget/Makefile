#	$NetBSD: Makefile,v 1.5.2.3 2013/01/16 05:27:59 yamt Exp $

LIB=	LLVMTarget

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target

SRCS+=	Mangler.cpp \
	Target.cpp \
	TargetInstrInfo.cpp \
	TargetIntrinsicInfo.cpp \
	TargetJITInfo.cpp \
	TargetLibraryInfo.cpp \
	TargetLoweringObjectFile.cpp \
	TargetMachine.cpp \
	TargetMachineC.cpp \
	TargetRegisterInfo.cpp \
	TargetSubtargetInfo.cpp \
	TargetTransformImpl.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
