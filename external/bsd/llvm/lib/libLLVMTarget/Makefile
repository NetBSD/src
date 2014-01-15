#	$NetBSD: Makefile,v 1.12 2014/01/15 22:21:35 joerg Exp $

LIB=	LLVMTarget

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target

SRCS+=	Target.cpp \
	TargetIntrinsicInfo.cpp \
	TargetJITInfo.cpp \
	TargetLibraryInfo.cpp \
	TargetLoweringObjectFile.cpp \
	TargetMachine.cpp \
	TargetMachineC.cpp \
	TargetSubtargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
