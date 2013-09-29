#	$NetBSD: Makefile,v 1.1 2013/09/29 12:11:59 joerg Exp $

PROG_CXX=	lldb
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/tools/driver

SRCS=	Driver.cpp \
	IOChannel.cpp

CPPFLAGS.Driver.cpp+=	-Dgetopt_long_only=getopt_long

LLDB_LIBS+= \
	API \
	Log \
	PluginInstructionARM \
	PluginProcessGDBRemote \
	PluginProcessMachCore \
	PluginProcessPOSIX \
	PluginProcessUtility \
	PluginSymbolFileDWARF \
	PluginSymbolFileSymtab \
	PluginSymbolVendorELF \
	PluginUnwindAssemblyInstEmulation \
	PluginUnwindAssemblyX86 \
	Breakpoint \
	Expression \
	Symbol \
	Target \
	Utility \
	HostCommon \
	Interpreter \
	Commands \
	Breakpoint \
	Core \
	DataFormatters \
	PluginABIMacOSX_arm \
	PluginABIMacOSX_i386 \
	PluginABISysV_x86_64 \
	PluginAppleObjCRuntime \
	PluginCXXItaniumABI \
	PluginDisassemblerLLVM \
	PluginDynamicLoaderMacOSXDYLD \
	PluginDynamicLoaderPosixDYLD \
	PluginDynamicLoaderStatic \
	PluginObjectContainerBSDArchive \
	PluginObjectContainerMachOArchive \
	PluginObjectFileELF \
	PluginObjectFileMachO \
	PluginObjectFilePECOFF \
	PluginPlatformNetBSD \
	PluginPlatformGDB \
	PluginPlatformLinux \
	PluginPlatformMacOSX \
	PluginPlatformPOSIX \
	PluginProcessElfCore \
	PluginProcessNetBSD \
	PluginProcessPOSIX \
	PluginProcessUtility \
	HostCommon \
	HostNetBSD \
	Interpreter \

CLANG_LIBS+= \
	clangFrontend \
	clangDriver \
	clangSerialization \
	clangCodeGen \
	clangParse \
	clangSema \
	clangEdit \
	clangAnalysis \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	JIT \
	MCJIT \
	RuntimeDyld \
	ExecutionEngine \
	ObjCARC \
	ipo \
	IRReader \
	AsmParser \
	BitReader \
	BitWriter \
	MCDisassembler \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Disassembler \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	ARMDisassembler \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsDisassembler \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCAsmParser \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmParser \
	X86Disassembler \
	X86AsmPrinter \
	X86Utils \
	Object \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	MCDisassembler \
	MCParser \
	MC \
	Linker \
	TransformsUtils \
	Analysis \
	ipa \
	IR \
	Option \
	Support

LDADD+=	-ledit -lterminfo -lexecinfo
DPADD+=	${LIBEDIT} ${LIBTERMINFO} ${LIBEXECINFO}
DBG=-g

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
