#	$NetBSD: Makefile,v 1.11.2.1 2018/07/28 04:33:05 pgoyette Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp \
	ARMWinEHPrinter.cpp \
	COFFDumper.cpp \
	COFFImportDumper.cpp \
	ELFDumper.cpp \
	Error.cpp \
	MachODumper.cpp \
	ObjDumper.cpp \
	WasmDumper.cpp \
	Win64EHDumper.cpp \
	WindowsResourceDumper.cpp

LLVM_LIBS+= \
	DebugInfoDWARF \
	AArch64TargetInfo \
	ARMTargetInfo \
	MipsTargetInfo \
	PowerPCTargetInfo \
	SparcTargetInfo \
	X86TargetInfo \
	DebugInfoCodeView \
	DebugInfoMSF \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
