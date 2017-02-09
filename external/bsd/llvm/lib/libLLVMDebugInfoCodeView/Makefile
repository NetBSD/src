#	$NetBSD: Makefile,v 1.3 2017/02/09 17:58:57 joerg Exp $

LIB=	LLVMDebugInfoCodeView

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/CodeView

SRCS+=	CVSymbolVisitor.cpp \
	CVTypeDumper.cpp \
	CVTypeVisitor.cpp \
	CodeViewError.cpp \
	CodeViewRecordIO.cpp \
	EnumTables.cpp \
	Line.cpp \
	ModuleSubstream.cpp \
	ModuleSubstreamVisitor.cpp \
	RecordSerialization.cpp \
	SymbolDumper.cpp \
	SymbolRecordMapping.cpp \
	TypeDatabase.cpp \
	TypeDatabaseVisitor.cpp \
	TypeDumpVisitor.cpp \
	TypeRecord.cpp \
	TypeRecordMapping.cpp \
	TypeSerializer.cpp \
	TypeStreamMerger.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
