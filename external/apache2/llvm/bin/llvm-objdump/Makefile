#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:48 joerg Exp $

PROG_CXX=	llvm-objdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-objdump

SRCS=	llvm-objdump.cpp \
	COFFDump.cpp \
	ELFDump.cpp \
	MachODump.cpp \
	SourcePrinter.cpp \
	WasmDump.cpp \
	XCOFFDump.cpp

TABLEGEN_SRC=	ObjdumpOpts.td OtoolOpts.td

TABLEGEN_OUTPUT.ObjdumpOpts.td= \
	ObjdumpOpts.inc|-gen-opt-parser-defs

TABLEGEN_OUTPUT.OtoolOpts.td= \
	OtoolOpts.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../../tablegen.mk"

LLVM_LIBS+= \
	AArch64Disassembler \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64TargetInfo \
	AArch64Utils \
	AMDGPUDisassembler \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUAsmParser \
	AMDGPUUtils \
	ARMDisassembler \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMTargetInfo \
	ARMUtils \
	MipsDisassembler \
	MipsAsmParser \
	MipsMCTargetDesc \
	MipsTargetInfo \
	PowerPCDisassembler \
	PowerPCAsmParser \
	PowerPCMCTargetDesc \
	PowerPCTargetInfo \
	SparcDisassembler \
	SparcAsmParser \
	SparcMCTargetDesc \
	SparcTargetInfo \
	X86Disassembler \
	X86AsmParser \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	CodeGen \
	MCDisassembler \
	DebugInfoDWARF \
	DebugInfoSymbolize \
	DebugInfoPDB \
	Object \
	TextAPI \
	BitReader \
	BitstreamReader \
	MCParser \
	MC \
	IR \
	Remarks \
	BinaryFormat \
	Option \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
