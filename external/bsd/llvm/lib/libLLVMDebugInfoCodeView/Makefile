#	$NetBSD: Makefile,v 1.2.2.1 2017/04/21 16:52:02 bouyer Exp $

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
