#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:51 joerg Exp $

LIB=	LLVMDebugInfoPDB

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB

SRCS+=	GenericError.cpp \
	IPDBSourceFile.cpp \
	PDB.cpp \
	PDBContext.cpp \
	PDBExtras.cpp \
	PDBInterfaceAnchors.cpp \
	PDBSymbol.cpp \
	PDBSymbolAnnotation.cpp \
	PDBSymbolBlock.cpp \
	PDBSymbolCompiland.cpp \
	PDBSymbolCompilandDetails.cpp \
	PDBSymbolCompilandEnv.cpp \
	PDBSymbolCustom.cpp \
	PDBSymbolData.cpp \
	PDBSymbolExe.cpp \
	PDBSymbolFunc.cpp \
	PDBSymbolFuncDebugEnd.cpp \
	PDBSymbolFuncDebugStart.cpp \
	PDBSymbolLabel.cpp \
	PDBSymbolPublicSymbol.cpp \
	PDBSymbolThunk.cpp \
	PDBSymbolTypeArray.cpp \
	PDBSymbolTypeBaseClass.cpp \
	PDBSymbolTypeBuiltin.cpp \
	PDBSymbolTypeCustom.cpp \
	PDBSymbolTypeDimension.cpp \
	PDBSymbolTypeEnum.cpp \
	PDBSymbolTypeFriend.cpp \
	PDBSymbolTypeFunctionArg.cpp \
	PDBSymbolTypeFunctionSig.cpp \
	PDBSymbolTypeManaged.cpp \
	PDBSymbolTypePointer.cpp \
	PDBSymbolTypeTypedef.cpp \
	PDBSymbolTypeUDT.cpp \
	PDBSymbolTypeVTable.cpp \
	PDBSymbolTypeVTableShape.cpp \
	PDBSymbolUnknown.cpp \
	PDBSymbolUsingNamespace.cpp \
	PDBSymDumper.cpp \
	UDTLayout.cpp

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
	NativeEnumLineNumbers.cpp \
	NativeEnumModules.cpp \
	NativeEnumSymbols.cpp \
	NativeEnumTypes.cpp \
	NativeExeSymbol.cpp \
	NativeFunctionSymbol.cpp \
	NativeInlineSiteSymbol.cpp \
	NativeLineNumber.cpp \
	NativePublicSymbol.cpp \
	NativeRawSymbol.cpp \
	NativeSession.cpp \
	NativeSourceFile.cpp \
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
