#	$NetBSD: Makefile,v 1.1 2017/08/01 20:07:41 joerg Exp $

LIB=	LLVMDebugInfoPDBNative

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB/Native

SRCS+=	DbiModuleDescriptor.cpp \
	DbiModuleDescriptorBuilder.cpp \
	DbiModuleList.cpp \
	DbiStreamBuilder.cpp \
	DbiStream.cpp \
	EnumTables.cpp \
	GlobalsStream.cpp \
	GSI.cpp \
	Hash.cpp \
	HashTable.cpp \
	InfoStreamBuilder.cpp \
	InfoStream.cpp \
	ModuleDebugStream.cpp \
	NamedStreamMap.cpp \
	NativeBuiltinSymbol.cpp \
	NativeCompilandSymbol.cpp \
	NativeEnumModules.cpp \
	NativeExeSymbol.cpp \
	NativeRawSymbol.cpp \
	NativeSession.cpp \
	PDBFileBuilder.cpp \
	PDBFile.cpp \
	PDBStringTable.cpp \
	PDBStringTableBuilder.cpp \
	PublicsStream.cpp \
	PublicsStreamBuilder.cpp \
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
