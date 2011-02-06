#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:01 joerg Exp $

LIB=	LLVMSelectionDAG

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/SelectionDAG

SRCS+=	DAGCombiner.cpp \
	FastISel.cpp \
	FunctionLoweringInfo.cpp \
	InstrEmitter.cpp \
	LegalizeDAG.cpp \
	LegalizeFloatTypes.cpp \
	LegalizeIntegerTypes.cpp \
	LegalizeTypes.cpp \
	LegalizeTypesGeneric.cpp \
	LegalizeVectorOps.cpp \
	LegalizeVectorTypes.cpp \
	ScheduleDAGFast.cpp \
	ScheduleDAGList.cpp \
	ScheduleDAGRRList.cpp \
	ScheduleDAGSDNodes.cpp \
	SelectionDAG.cpp \
	SelectionDAGBuilder.cpp \
	SelectionDAGISel.cpp \
	SelectionDAGPrinter.cpp \
	TargetLowering.cpp \
	TargetSelectionDAGInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
