#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:48 joerg Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp \
	ARMWinEHPrinter.cpp \
	COFFDumper.cpp \
	COFFImportDumper.cpp \
	ELFDumper.cpp \
	MachODumper.cpp \
	ObjDumper.cpp \
	WasmDumper.cpp \
	Win64EHDumper.cpp \
	WindowsResourceDumper.cpp \
	XCOFFDumper.cpp

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

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
