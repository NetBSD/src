#	$NetBSD: Makefile,v 1.4 2011/08/02 17:12:39 joerg Exp $

LIB=	LLVMSupport

NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Support

SRCS+=	APFloat.cpp \
	APInt.cpp \
	APSInt.cpp \
	Allocator.cpp \
	BlockFrequency.cpp \
	BranchProbability.cpp \
	circular_raw_ostream.cpp \
	CommandLine.cpp \
	ConstantRange.cpp \
	CrashRecoveryContext.cpp \
	Debug.cpp \
	DeltaAlgorithm.cpp \
	DAGDeltaAlgorithm.cpp \
	Dwarf.cpp \
	ErrorHandling.cpp \
	FileUtilities.cpp \
	FoldingSet.cpp \
	FormattedStream.cpp \
	GraphWriter.cpp \
	IntEqClasses.cpp \
	IntervalMap.cpp \
	IsInf.cpp \
	IsNAN.cpp \
	ManagedStatic.cpp \
	MemoryBuffer.cpp \
	MemoryObject.cpp \
	PluginLoader.cpp \
	PrettyStackTrace.cpp \
	Regex.cpp \
	SmallPtrSet.cpp \
	SmallVector.cpp \
	SourceMgr.cpp \
	Statistic.cpp \
	StringExtras.cpp \
	StringMap.cpp \
	StringPool.cpp \
	StringRef.cpp \
	SystemUtils.cpp \
	Timer.cpp \
	ToolOutputFile.cpp \
	Triple.cpp \
	Twine.cpp \
	raw_os_ostream.cpp \
	raw_ostream.cpp \
	regcomp.c \
	regerror.c \
	regexec.c \
	regfree.c \
	regstrlcpy.c

SRCS+=	Atomic.cpp \
	Disassembler.cpp \
	DynamicLibrary.cpp \
	Errno.cpp \
	Host.cpp \
	IncludeFile.cpp \
	Memory.cpp \
	Mutex.cpp \
	Path.cpp \
	PathV2.cpp \
	Process.cpp \
	Program.cpp \
	RWMutex.cpp \
	SearchForAddressOfSpecialSymbol.cpp \
	Signals.cpp \
	system_error.cpp \
	ThreadLocal.cpp \
	Threading.cpp \
	TimeValue.cpp \
	Valgrind.cpp

.PATH: ${LLVM_SRCDIR}/lib/Support/Unix

SRCS+=	Host.inc \
	Memory.inc \
	Mutex.inc \
	Path.inc \
	PathV2.inc \
	Process.inc \
	Program.inc \
	RWMutex.inc \
	Signals.inc \
	system_error.inc \
	ThreadLocal.inc \
	TimeValue.inc

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
