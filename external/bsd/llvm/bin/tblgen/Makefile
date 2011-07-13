#	$NetBSD: Makefile,v 1.6 2011/07/13 23:48:29 joerg Exp $

PROG_CXX=	llvm-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/utils/TableGen

SRCS=	ARMDecoderEmitter.cpp \
	AsmMatcherEmitter.cpp \
	AsmWriterEmitter.cpp \
	AsmWriterInst.cpp \
	CallingConvEmitter.cpp \
	ClangASTNodesEmitter.cpp \
	ClangAttrEmitter.cpp \
	ClangDiagnosticsEmitter.cpp \
	ClangSACheckersEmitter.cpp \
	CodeEmitterGen.cpp \
	CodeGenDAGPatterns.cpp \
	CodeGenInstruction.cpp \
	CodeGenRegisters.cpp \
	CodeGenTarget.cpp \
	DAGISelEmitter.cpp \
	DAGISelMatcher.cpp \
	DAGISelMatcherEmitter.cpp \
	DAGISelMatcherGen.cpp \
	DAGISelMatcherOpt.cpp \
	DisassemblerEmitter.cpp \
	EDEmitter.cpp \
	Error.cpp \
	FastISelEmitter.cpp \
	FixedLenDecoderEmitter.cpp \
	FixedLenDecoderEmitter.h \
	InstrEnumEmitter.cpp \
	InstrInfoEmitter.cpp \
	IntrinsicEmitter.cpp \
	LLVMCConfigurationEmitter.cpp \
	NeonEmitter.cpp \
	OptParserEmitter.cpp \
	PseudoLoweringEmitter.cpp \
	Record.cpp \
	RegisterInfoEmitter.cpp \
	SetTheory.cpp \
	StringMatcher.cpp \
	SubtargetEmitter.cpp \
	TGLexer.cpp \
	TGParser.cpp \
	TGValueTypes.cpp \
	TableGen.cpp \
	TableGenBackend.cpp \
	X86DisassemblerTables.cpp \
	X86RecognizableInstr.cpp

LLVM_LIBS+= Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
