#	$NetBSD: Makefile,v 1.2 2011/02/23 03:09:04 joerg Exp $

LIB=	LLVMTarget

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target

SRCS+=	Mangler.cpp \
	SubtargetFeature.cpp \
	Target.cpp \
	TargetAsmInfo.cpp \
	TargetAsmLexer.cpp \
	TargetData.cpp \
	TargetELFWriterInfo.cpp \
	TargetFrameLowering.cpp \
	TargetInstrInfo.cpp \
	TargetIntrinsicInfo.cpp \
	TargetLibraryInfo.cpp \
	TargetLoweringObjectFile.cpp \
	TargetMachine.cpp \
	TargetRegisterInfo.cpp \
	TargetSubtarget.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
