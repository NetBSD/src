#	$NetBSD: Makefile,v 1.16.2.1 2018/07/28 04:37:13 pgoyette Exp $

LIB=	clangBasic

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Basic

SRCS+=	Attributes.cpp \
	Builtins.cpp \
	CharInfo.cpp \
	Cuda.cpp \
	Diagnostic.cpp \
	DiagnosticIDs.cpp \
	DiagnosticOptions.cpp \
	FileManager.cpp \
	FileSystemStatCache.cpp \
	IdentifierTable.cpp \
	LangOptions.cpp \
	MemoryBufferCache.cpp \
	Module.cpp \
	ObjCRuntime.cpp \
	OpenMPKinds.cpp \
	OperatorPrecedence.cpp \
	SanitizerBlacklist.cpp \
	SanitizerSpecialCaseList.cpp \
	Sanitizers.cpp \
	SourceLocation.cpp \
	SourceManager.cpp \
	TargetInfo.cpp \
	Targets.cpp \
	TokenKinds.cpp \
	Version.cpp \
	VirtualFileSystem.cpp \
	Warnings.cpp \
	XRayInstr.cpp \
	XRayLists.cpp

.PATH: ${CLANG_SRCDIR}/lib/Basic/Targets

SRCS+=	AArch64.cpp \
	AMDGPU.cpp \
	ARM.cpp \
	AVR.cpp \
	BPF.cpp \
	Hexagon.cpp \
	Lanai.cpp \
	Le64.cpp \
	MSP430.cpp \
	Mips.cpp \
	NVPTX.cpp \
	Nios2.cpp \
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

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/Basic

CPPFLAGS.Version.cpp=	-DSVN_REVISION=\"${CLANG_REVISION}\" \
			-DLLVM_REVISION=\"${LLVM_REVISION}\"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
