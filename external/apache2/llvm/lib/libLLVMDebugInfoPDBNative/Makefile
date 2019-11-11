#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:06 joerg Exp $

LIB=	LLVMDebugInfoPDBNative

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB/Native

SRCS+=	DbiModuleDescriptorBuilder.cpp \
	DbiModuleDescriptor.cpp \
	DbiModuleList.cpp \
	DbiStreamBuilder.cpp \
	DbiStream.cpp \
	EnumTables.cpp \
	GlobalsStream.cpp \
	GSIStreamBuilder.cpp \
	Hash.cpp \
	HashTable.cpp \
	InfoStreamBuilder.cpp \
	InfoStream.cpp \
	InjectedSourceStream.cpp \
	ModuleDebugStream.cpp \
	NamedStreamMap.cpp \
	NativeCompilandSymbol.cpp \
	NativeEnumGlobals.cpp \
	NativeEnumInjectedSources.cpp \
	NativeEnumModules.cpp \
	NativeEnumTypes.cpp \
	NativeExeSymbol.cpp \
	NativeRawSymbol.cpp \
	NativeSession.cpp \
	NativeSymbolEnumerator.cpp \
	NativeTypeArray.cpp \
	NativeTypeBuiltin.cpp \
	NativeTypeEnum.cpp \
	NativeTypeFunctionSig.cpp \
	NativeTypePointer.cpp \
	NativeTypeTypedef.cpp \
	NativeTypeUDT.cpp \
	NativeTypeVTShape.cpp \
	PDBFileBuilder.cpp \
	PDBFile.cpp \
	PDBStringTableBuilder.cpp \
	PDBStringTable.cpp \
	PublicsStream.cpp \
	RawError.cpp \
	SymbolCache.cpp \
	SymbolStream.cpp \
	TpiHashing.cpp \
	TpiStreamBuilder.cpp \
	TpiStream.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
