#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:02 maya Exp $

.include <bsd.init.mk>

SUBDIR+=	IR

.ifndef HOST_BUILD
SUBDIR+=	ADT
SUBDIR+=	Analysis
SUBDIR+=	AsmParser
SUBDIR+=	BinaryFormat
SUBDIR+=	Bitcode
SUBDIR+=	Config
SUBDIR+=	CodeGen
SUBDIR+=	DebugInfo
SUBDIR+=	Demangle
SUBDIR+=	FuzzMutate
SUBDIR+=	ExecutionEngine
SUBDIR+=	IRReader
SUBDIR+=	LTO
SUBDIR+=	LineEditor
SUBDIR+=	Linker
SUBDIR+=	MC
SUBDIR+=	Object
SUBDIR+=	ObjectYAML
SUBDIR+=	Option
SUBDIR+=	Passes
SUBDIR+=	ProfileData
SUBDIR+=	Support
SUBDIR+=	TableGen
SUBDIR+=	Target
SUBDIR+=	Transforms
SUBDIR+=	WindowsManifest
SUBDIR+=	WindowsResource
SUBDIR+=	XRay

.PATH:	${LLVM_SRCDIR}/include/llvm

INCS=	InitializePasses.h \
	LinkAllIR.h \
	LinkAllPasses.h \
	Pass.h \
	PassAnalysisSupport.h \
	PassInfo.h \
	PassRegistry.h \
	PassSupport.h

INCSDIR=	/usr/include/llvm

.endif

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
