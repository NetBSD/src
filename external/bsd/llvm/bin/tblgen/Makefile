#	$NetBSD: Makefile,v 1.3.4.1 2011/06/23 14:18:30 cherry Exp $

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
	CodeGenTarget.cpp \
	DAGISelEmitter.cpp \
	DAGISelMatcher.cpp \
	DAGISelMatcherEmitter.cpp \
	DAGISelMatcherGen.cpp \
	DAGISelMatcherOpt.cpp \
	DisassemblerEmitter.cpp \
	EDEmitter.cpp \
	FastISelEmitter.cpp \
	FixedLenDecoderEmitter.cpp \
	FixedLenDecoderEmitter.h \
	InstrEnumEmitter.cpp \
	InstrInfoEmitter.cpp \
	IntrinsicEmitter.cpp \
	LLVMCConfigurationEmitter.cpp \
	NeonEmitter.cpp \
	OptParserEmitter.cpp \
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
