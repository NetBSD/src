#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:00 pgoyette Exp $

LIB=	LLVMDebugInfoPDBRaw

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB/Raw

SRCS+=	DbiStreamBuilder.cpp \
	DbiStream.cpp \
	EnumTables.cpp \
	GSI.cpp \
	GlobalsStream.cpp \
	Hash.cpp \
	InfoStreamBuilder.cpp \
	InfoStream.cpp \
	ModInfo.cpp \
	ModStream.cpp \
	NameHashTable.cpp \
	NameMapBuilder.cpp \
	NameMap.cpp \
	PDBFileBuilder.cpp \
	PDBFile.cpp \
	PublicsStream.cpp \
	RawError.cpp \
	RawSession.cpp \
	SymbolStream.cpp \
	TpiHashing.cpp \
	TpiStreamBuilder.cpp \
	TpiStream.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
