#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:02 joerg Exp $

PROG_CXX=	llvm-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/utils/TableGen

SRCS=	AsmMatcherEmitter.cpp \
	AsmWriterEmitter.cpp \
	AsmWriterInst.cpp \
	Attributes.cpp \
	CallingConvEmitter.cpp \
	CodeEmitterGen.cpp \
	CodeGenDAGPatterns.cpp \
	CodeGenHwModes.cpp \
	CodeGenInstruction.cpp \
	CodeGenMapTable.cpp \
	CodeGenRegisters.cpp \
	CodeGenSchedule.cpp \
	CodeGenTarget.cpp \
	CTagsEmitter.cpp \
	DAGISelEmitter.cpp \
	DAGISelMatcher.cpp \
	DAGISelMatcherEmitter.cpp \
	DAGISelMatcherGen.cpp \
	DAGISelMatcherOpt.cpp \
	DFAEmitter.cpp \
	DFAPacketizerEmitter.cpp \
	DisassemblerEmitter.cpp \
	ExegesisEmitter.cpp \
	FastISelEmitter.cpp \
	FixedLenDecoderEmitter.cpp \
	GICombinerEmitter.cpp \
	GlobalISelEmitter.cpp \
	InfoByHwMode.cpp \
	InstrDocsEmitter.cpp \
	InstrInfoEmitter.cpp \
	IntrinsicEmitter.cpp \
	OptParserEmitter.cpp \
	PredicateExpander.cpp \
	PseudoLoweringEmitter.cpp \
	RegisterBankEmitter.cpp \
	RegisterInfoEmitter.cpp \
	RISCVCompressInstEmitter.cpp \
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

.PATH: ${LLVM_SRCDIR}/utils/TableGen/GlobalISel
SRCS+=	CodeExpander.cpp

LLVM_LIBS+= \
	TableGen \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
