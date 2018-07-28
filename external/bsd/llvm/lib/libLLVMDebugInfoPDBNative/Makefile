#	$NetBSD: Makefile,v 1.1.2.1 2018/07/28 04:37:11 pgoyette Exp $

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
	ModuleDebugStream.cpp \
	NamedStreamMap.cpp \
	NativeBuiltinSymbol.cpp \
	NativeCompilandSymbol.cpp \
	NativeEnumModules.cpp \
	NativeEnumSymbol.cpp \
	NativeEnumTypes.cpp \
	NativeExeSymbol.cpp \
	NativeRawSymbol.cpp \
	NativeSession.cpp \
	PDBFileBuilder.cpp \
	PDBFile.cpp \
	PDBStringTableBuilder.cpp \
	PDBStringTable.cpp \
	PublicsStream.cpp \
	RawError.cpp \
	SymbolStream.cpp \
	TpiHashing.cpp \
	TpiStreamBuilder.cpp \
	TpiStream.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
