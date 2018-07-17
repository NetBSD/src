#	$NetBSD: Makefile,v 1.28 2018/07/17 18:55:30 joerg Exp $

LIB=	clangStaticAnalyzerCheckers

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Checkers
.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Checkers/MPI-Checker

SRCS+=	AllocationDiagnostics.cpp \
	AnalysisOrderChecker.cpp \
	AnalyzerStatsChecker.cpp \
	ArrayBoundChecker.cpp \
	ArrayBoundCheckerV2.cpp \
	BasicObjCFoundationChecks.cpp \
	BlockInCriticalSectionChecker.cpp \
	BoolAssignmentChecker.cpp \
	BuiltinFunctionChecker.cpp \
	CallAndMessageChecker.cpp \
	CastSizeChecker.cpp \
	CastToStructChecker.cpp \
	CheckerDocumentation.cpp \
	CheckObjCDealloc.cpp \
	CheckObjCInstMethSignature.cpp \
	CheckSecuritySyntaxOnly.cpp \
	CheckSizeofPointer.cpp \
	ChrootChecker.cpp \
	ClangCheckers.cpp \
	CloneChecker.cpp \
	ConversionChecker.cpp \
	CStringChecker.cpp \
	CStringSyntaxChecker.cpp \
	CXXSelfAssignmentChecker.cpp \
	DanglingInternalBufferChecker.cpp \
	DeadStoresChecker.cpp \
	DebugCheckers.cpp \
	DeleteWithNonVirtualDtorChecker.cpp \
	DereferenceChecker.cpp \
	DirectIvarAssignment.cpp \
	DivZeroChecker.cpp \
	DynamicTypeChecker.cpp \
	DynamicTypePropagation.cpp \
	ExprInspectionChecker.cpp \
	FixedAddressChecker.cpp \
	GCDAntipatternChecker.cpp \
	GenericTaintChecker.cpp \
	GTestChecker.cpp \
	IdenticalExprChecker.cpp \
	IteratorChecker.cpp \
	IvarInvalidationChecker.cpp \
	LLVMConventionsChecker.cpp \
	LocalizationChecker.cpp \
	MacOSKeychainAPIChecker.cpp \
	MacOSXAPIChecker.cpp \
	MallocChecker.cpp \
	MallocOverflowSecurityChecker.cpp \
	MallocSizeofChecker.cpp \
	MisusedMovedObjectChecker.cpp \
	MmapWriteExecChecker.cpp \
	MPIBugReporter.cpp \
	MPIChecker.cpp \
	MPIFunctionClassifier.cpp \
	NonnullGlobalConstantsChecker.cpp \
	NonNullParamChecker.cpp \
	NoReturnFunctionChecker.cpp \
	NSAutoreleasePoolChecker.cpp \
	NSErrorChecker.cpp \
	NullabilityChecker.cpp \
	NumberObjectConversionChecker.cpp \
	ObjCAtSyncChecker.cpp \
	ObjCAutoreleaseWriteChecker.cpp \
	ObjCContainersASTChecker.cpp \
	ObjCContainersChecker.cpp \
	ObjCMissingSuperCallChecker.cpp \
	ObjCPropertyChecker.cpp \
	ObjCSelfInitChecker.cpp \
	ObjCSuperDeallocChecker.cpp \
	ObjCUnusedIVarsChecker.cpp \
	PaddingChecker.cpp \
	PointerArithChecker.cpp \
	PointerSubChecker.cpp \
	PthreadLockChecker.cpp \
	RetainCountChecker.cpp \
	ReturnPointerRangeChecker.cpp \
	ReturnUndefChecker.cpp \
	SimpleStreamChecker.cpp \
	StackAddrEscapeChecker.cpp \
	StdLibraryFunctionsChecker.cpp \
	StreamChecker.cpp \
	TaintTesterChecker.cpp \
	TestAfterDivZeroChecker.cpp \
	TraversalChecker.cpp \
	TrustNonnullChecker.cpp \
	UndefBranchChecker.cpp \
	UndefCapturedBlockVarChecker.cpp \
	UndefinedArraySubscriptChecker.cpp \
	UndefinedAssignmentChecker.cpp \
	UndefResultChecker.cpp \
	UninitializedObjectChecker.cpp \
	UnixAPIChecker.cpp \
	UnreachableCodeChecker.cpp \
	ValistChecker.cpp \
	VforkChecker.cpp \
	VirtualCallChecker.cpp \
	VLASizeChecker.cpp

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
