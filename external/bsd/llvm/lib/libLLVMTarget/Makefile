#	$NetBSD: Makefile,v 1.12.4.1 2015/06/04 20:05:45 snj Exp $

LIB=	LLVMTarget

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target

SRCS+=	Target.cpp \
	TargetIntrinsicInfo.cpp \
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
