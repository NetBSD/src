#	$NetBSD: Makefile,v 1.15 2013/02/04 15:00:35 joerg Exp $

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
	ConvertUTF.c \
	ConvertUTFWrapper.cpp \
	CrashRecoveryContext.cpp \
	DataExtractor.cpp \
	DataStream.cpp \
	Debug.cpp \
	DeltaAlgorithm.cpp \
	DAGDeltaAlgorithm.cpp \
	Dwarf.cpp \
	ErrorHandling.cpp \
	FileOutputBuffer.cpp \
	FileUtilities.cpp \
	FoldingSet.cpp \
	FormattedStream.cpp \
	GraphWriter.cpp \
	Hashing.cpp \
	IntEqClasses.cpp \
	IntervalMap.cpp \
	IntrusiveRefCntPtr.cpp \
	IsInf.cpp \
	IsNAN.cpp \
	Locale.cpp \
	LockFileManager.cpp \
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
	StreamableMemoryObject.cpp \
	StringExtras.cpp \
	StringMap.cpp \
	StringPool.cpp \
	StringRef.cpp \
	SystemUtils.cpp \
	TargetRegistry.cpp \
	Timer.cpp \
	ToolOutputFile.cpp \
	Triple.cpp \
	Twine.cpp \
	YAMLParser.cpp \
	YAMLTraits.cpp \
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
