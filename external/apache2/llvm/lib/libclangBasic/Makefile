#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangBasic

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Basic

SRCS+=	Attributes.cpp \
	Builtins.cpp \
	CharInfo.cpp \
	CodeGenOptions.cpp \
	Cuda.cpp \
	Diagnostic.cpp \
	DiagnosticIDs.cpp \
	DiagnosticOptions.cpp \
	ExpressionTraits.cpp \
	FileEntry.cpp \
	FileManager.cpp \
	FileSystemStatCache.cpp \
	IdentifierTable.cpp \
	LangOptions.cpp \
	LangStandards.cpp \
	Module.cpp \
	NoSanitizeList.cpp \
	ObjCRuntime.cpp \
	OpenCLOptions.cpp \
	OpenMPKinds.cpp \
	OperatorPrecedence.cpp \
	ProfileList.cpp \
	Sanitizers.cpp \
	SanitizerSpecialCaseList.cpp \
	SourceLocation.cpp \
	SourceManager.cpp \
	Stack.cpp \
	TargetID.cpp \
	TargetInfo.cpp \
	Targets.cpp \
	TokenKinds.cpp \
	TypeTraits.cpp \
	VE.cpp \
	Version.cpp \
	Warnings.cpp \
	XRayInstr.cpp \
	XRayLists.cpp

.PATH: ${CLANG_SRCDIR}/lib/Basic/Targets

SRCS+=	AArch64.cpp \
	AMDGPU.cpp \
	ARC.cpp \
	ARM.cpp \
	AVR.cpp \
	BPF.cpp \
	Hexagon.cpp \
	Lanai.cpp \
	Le64.cpp \
	M68k.cpp \
	MSP430.cpp \
	Mips.cpp \
	NVPTX.cpp \
	OSTargets.cpp \
	PNaCl.cpp \
	PPC.cpp \
	RISCV.cpp \
	SPIR.cpp \
	Sparc.cpp \
	SystemZ.cpp \
	TCE.cpp \
	WebAssembly.cpp \
	X86.cpp \
	XCore.cpp

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/Basic -I.

VCSVersion.inc: ${LLVM_TOPLEVEL}/Makefile.inc
	echo '#define LLVM_REVISION "${LLVM_REVISION}"' > VCSVersion.inc.tmp
	echo '#define LLVM_REPOSITORY "git@github.com:llvm/llvm-project.git"' >> VCSVersion.inc.tmp
	echo '#define CLANG_REVISION "${CLANG_REVISION}"' >> VCSVersion.inc.tmp
	echo '#define CLANG_REPOSITORY "git@github.com:llvm/llvm-project.git"' >> VCSVersion.inc.tmp
	mv VCSVersion.inc.tmp VCSVersion.inc

DPSRCS+=	VCSVersion.inc
CLEANFILES+=	VCSVersion.inc

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
