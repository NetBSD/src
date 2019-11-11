#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:08 joerg Exp $

LIB=	LLVMMCA

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MCA

SRCS+=	CodeEmitter.cpp \
	Context.cpp \
	HWEventListener.cpp \
	InstrBuilder.cpp \
	Instruction.cpp \
	Pipeline.cpp \
	Support.cpp

.PATH: ${LLVM_SRCDIR}/lib/MCA/HardwareUnits

SRCS+=	HardwareUnit.cpp \
	LSUnit.cpp \
	RegisterFile.cpp \
	ResourceManager.cpp \
	RetireControlUnit.cpp \
	Scheduler.cpp

.PATH: ${LLVM_SRCDIR}/lib/MCA/Stages

SRCS+=	DispatchStage.cpp \
	EntryStage.cpp \
	ExecuteStage.cpp \
	InstructionTables.cpp \
	MicroOpQueueStage.cpp \
	RetireStage.cpp \
	Stage.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
