#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:12 joerg Exp $

LIB=	LLVMSupport

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Support

SRCS+=	AArch64TargetParser.cpp \
	ABIBreak.cpp \
	Allocator.cpp \
	AMDGPUMetadata.cpp \
	APFloat.cpp \
	APInt.cpp \
	APSInt.cpp \
	ARMAttributeParser.cpp \
	ARMBuildAttrs.cpp \
	ARMTargetParser.cpp \
	ARMWinEH.cpp \
	BinaryStreamError.cpp \
	BinaryStreamReader.cpp \
	BinaryStreamRef.cpp \
	BinaryStreamWriter.cpp \
	BlockFrequency.cpp \
	BranchProbability.cpp \
	BuryPointer.cpp \
	CachePruning.cpp \
	Chrono.cpp \
	circular_raw_ostream.cpp \
	CodeGenCoverage.cpp \
	COM.cpp \
	CommandLine.cpp \
	Compression.cpp \
	ConvertUTF.cpp \
	ConvertUTFWrapper.cpp \
	CrashRecoveryContext.cpp \
	CRC.cpp \
	DAGDeltaAlgorithm.cpp \
	DataExtractor.cpp \
	DebugCounter.cpp \
	Debug.cpp \
	DeltaAlgorithm.cpp \
	DJB.cpp \
	Error.cpp \
	ErrorHandling.cpp \
	FileCheck.cpp \
	FileCollector.cpp \
	FileOutputBuffer.cpp \
	FileUtilities.cpp \
	FoldingSet.cpp \
	FormattedStream.cpp \
	GlobPattern.cpp \
	GraphWriter.cpp \
	Hashing.cpp \
	InitLLVM.cpp \
	IntEqClasses.cpp \
	IntervalMap.cpp \
	ItaniumManglingCanonicalizer.cpp \
	JSON.cpp \
	KnownBits.cpp \
	LEB128.cpp \
	LineIterator.cpp \
	Locale.cpp \
	LockFileManager.cpp \
	LowLevelType.cpp \
	ManagedStatic.cpp \
	MathExtras.cpp \
	MD5.cpp \
	MemoryBuffer.cpp \
	NativeFormatting.cpp \
	Optional.cpp \
	Options.cpp \
	Parallel.cpp \
	PluginLoader.cpp \
	PrettyStackTrace.cpp \
	raw_os_ostream.cpp \
	raw_ostream.cpp \
	regcomp.c \
	regerror.c \
	Regex.cpp \
	regexec.c \
	regfree.c \
	regstrlcpy.c \
	ScopedPrinter.cpp \
	SHA1.cpp \
	Signposts.cpp \
	SmallPtrSet.cpp \
	SmallVector.cpp \
	SourceMgr.cpp \
	SpecialCaseList.cpp \
	Statistic.cpp \
	StringExtras.cpp \
	StringMap.cpp \
	StringPool.cpp \
	StringRef.cpp \
	StringSaver.cpp \
	SymbolRemappingReader.cpp \
	SystemUtils.cpp \
	TargetParser.cpp \
	TargetRegistry.cpp \
	TarWriter.cpp \
	TimeProfiler.cpp \
	Timer.cpp \
	ToolOutputFile.cpp \
	TrigramIndex.cpp \
	Triple.cpp \
	Twine.cpp \
	UnicodeCaseFold.cpp \
	Unicode.cpp \
	VersionTuple.cpp \
	VirtualFileSystem.cpp \
	Watchdog.cpp \
	WithColor.cpp \
	xxhash.cpp \
	YAMLParser.cpp \
	YAMLTraits.cpp \
	Z3Solver.cpp

SRCS+=	Atomic.cpp \
	DynamicLibrary.cpp \
	Errno.cpp \
	FormatVariadic.cpp \
	Host.cpp \
	Memory.cpp \
	Path.cpp \
	Process.cpp \
	Program.cpp \
	RWMutex.cpp \
	RandomNumberGenerator.cpp \
	ScaledNumber.cpp \
	Signals.cpp \
	ThreadLocal.cpp \
	ThreadPool.cpp \
	Threading.cpp \
	Valgrind.cpp

.PATH: ${LLVM_SRCDIR}/lib/Support/Unix

SRCS+=	Host.inc \
	Memory.inc \
	Path.inc \
	Process.inc \
	Program.inc \
	Signals.inc \
	Watchdog.inc \
	ThreadLocal.inc

.for src in ${SRCS:M*.c}
CPPFLAGS.${src}+=	-std=gnu99
.endfor

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
