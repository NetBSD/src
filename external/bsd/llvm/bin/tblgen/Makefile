#	$NetBSD: Makefile,v 1.23 2018/07/17 18:55:26 joerg Exp $

PROG_CXX=	llvm-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/utils/TableGen

SRCS=	AsmMatcherEmitter.cpp \
	AsmWriterEmitter.cpp \
	AsmWriterInst.cpp \
	Attributes.cpp \
	CTagsEmitter.cpp \
	CallingConvEmitter.cpp \
	CodeEmitterGen.cpp \
	CodeGenDAGPatterns.cpp \
	CodeGenHwModes.cpp \
	CodeGenInstruction.cpp \
	CodeGenMapTable.cpp \
	CodeGenRegisters.cpp \
	CodeGenSchedule.cpp \
	CodeGenTarget.cpp \
	DAGISelEmitter.cpp \
	DAGISelMatcher.cpp \
	DAGISelMatcherEmitter.cpp \
	DAGISelMatcherGen.cpp \
	DAGISelMatcherOpt.cpp \
	DFAPacketizerEmitter.cpp \
	DisassemblerEmitter.cpp \
	FastISelEmitter.cpp \
	FixedLenDecoderEmitter.cpp \
	GlobalISelEmitter.cpp \
	InfoByHwMode.cpp \
	InstrInfoEmitter.cpp \
	InstrDocsEmitter.cpp \
	IntrinsicEmitter.cpp \
	OptParserEmitter.cpp \
	PredicateExpander.cpp \
	PseudoLoweringEmitter.cpp \
	RISCVCompressInstEmitter.cpp \
	RegisterBankEmitter.cpp \
	RegisterInfoEmitter.cpp \
	SDNodeProperties.cpp \
	SearchableTableEmitter.cpp \
	SubtargetEmitter.cpp \
	SubtargetFeatureInfo.cpp \
	TableGen.cpp \
	Types.cpp \
	WebAssemblyDisassemblerEmitter.cpp \
	X86DisassemblerTables.cpp \
	X86EVEX2VEXTablesEmitter.cpp \
	X86FoldTablesEmitter.cpp \
	X86ModRMFilters.cpp \
	X86RecognizableInstr.cpp

LLVM_LIBS+= \
	TableGen \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
