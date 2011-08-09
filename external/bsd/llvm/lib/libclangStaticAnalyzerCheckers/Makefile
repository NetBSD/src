#	$NetBSD: Makefile,v 1.5 2011/08/09 12:30:47 joerg Exp $

LIB=	clangStaticAnalyzerCheckers

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Checkers

SRCS+=	AdjustedReturnValueChecker.cpp \
	AnalyzerStatsChecker.cpp \
	ArrayBoundChecker.cpp \
	ArrayBoundCheckerV2.cpp \
	AttrNonNullChecker.cpp \
	BasicObjCFoundationChecks.cpp \
	BuiltinFunctionChecker.cpp \
	ClangSACheckerProvider.cpp \
	CStringChecker.cpp \
	CallAndMessageChecker.cpp \
	CastSizeChecker.cpp \
	CastToStructChecker.cpp \
	CheckObjCDealloc.cpp \
	CheckObjCInstMethSignature.cpp \
	CheckSecuritySyntaxOnly.cpp \
	CheckSizeofPointer.cpp \
	ChrootChecker.cpp \
	DeadStoresChecker.cpp \
	DebugCheckers.cpp \
	DereferenceChecker.cpp \
	DivZeroChecker.cpp \
	FixedAddressChecker.cpp \
	IdempotentOperationChecker.cpp \
	IteratorsChecker.cpp \
	LLVMConventionsChecker.cpp \
	MacOSKeychainAPIChecker.cpp \
	MacOSXAPIChecker.cpp \
	MallocChecker.cpp \
	MallocOverflowSecurityChecker.cpp \
	NSAutoreleasePoolChecker.cpp \
	NSErrorChecker.cpp \
	NoReturnFunctionChecker.cpp \
	OSAtomicChecker.cpp \
	ObjCAtSyncChecker.cpp \
	ObjCSelfInitChecker.cpp \
	ObjCUnusedIVarsChecker.cpp \
	PointerArithChecker.cpp \
	PointerSubChecker.cpp \
	PthreadLockChecker.cpp \
	ReturnPointerRangeChecker.cpp \
	ReturnUndefChecker.cpp \
	StackAddrEscapeChecker.cpp \
	StreamChecker.cpp \
	UndefBranchChecker.cpp \
	UndefCapturedBlockVarChecker.cpp \
	UndefResultChecker.cpp \
	UndefinedArraySubscriptChecker.cpp \
	UndefinedAssignmentChecker.cpp \
	UnixAPIChecker.cpp \
	UnreachableCodeChecker.cpp \
	VLASizeChecker.cpp

TABLEGEN_SRC=		Checkers.td
TABLEGEN_INCLUDES=	-I${CLANG_SRCDIR}/include
TABLEGEN_OUTPUT=	Checkers.inc|-gen-clang-sa-checkers

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
