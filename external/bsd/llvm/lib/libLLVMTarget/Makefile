#	$NetBSD: Makefile,v 1.8.2.1 2012/11/20 02:58:02 tls Exp $

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
