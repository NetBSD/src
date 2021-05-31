#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:06:59 cjep Exp $

PROG_CXX=	llvm-size
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-reduce

SRCS=	DeltaManager.cpp \
	TestRunner.cpp \
	llvm-reduce.cpp

.PATH: ${LLVM_SRCDIR}/tools/llvm-reduce/deltas

SRCS+=	Delta.cpp \
	ReduceAliases.cpp \
	ReduceArguments.cpp \
	ReduceAttributes.cpp \
	ReduceBasicBlocks.cpp \
	ReduceFunctionBodies.cpp \
	ReduceFunctions.cpp \
	ReduceGlobalValues.cpp \
	ReduceGlobalVarInitializers.cpp \
	ReduceGlobalVars.cpp \
	ReduceInstructions.cpp \
	ReduceMetadata.cpp \
	ReduceModuleInlineAsm.cpp \
	ReduceOperandBundles.cpp \
	ReduceSpecialGlobals.cpp


LLVM_LIBS+= \
	Object \
	TextAPI \
	MCParser \
	MC \
	BitReader \
	BitstreamReader \
	IR \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
