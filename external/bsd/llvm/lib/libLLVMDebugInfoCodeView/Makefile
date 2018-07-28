#	$NetBSD: Makefile,v 1.4.2.1 2018/07/28 04:37:10 pgoyette Exp $

LIB=	LLVMDebugInfoCodeView

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/CodeView

SRCS+=	AppendingTypeTableBuilder.cpp \
	CodeViewError.cpp \
	CodeViewRecordIO.cpp \
	ContinuationRecordBuilder.cpp \
	CVSymbolVisitor.cpp \
	CVTypeVisitor.cpp \
	DebugChecksumsSubsection.cpp \
	DebugCrossExSubsection.cpp \
	DebugCrossImpSubsection.cpp \
	DebugFrameDataSubsection.cpp \
	DebugInlineeLinesSubsection.cpp \
	DebugLinesSubsection.cpp \
	DebugStringTableSubsection.cpp \
	DebugSubsection.cpp \
	DebugSubsectionRecord.cpp \
	DebugSubsectionVisitor.cpp \
	DebugSymbolsSubsection.cpp \
	EnumTables.cpp \
	Formatters.cpp \
	GlobalTypeTableBuilder.cpp \
	LazyRandomTypeCollection.cpp \
	Line.cpp \
	MergingTypeTableBuilder.cpp \
	RecordName.cpp \
	RecordSerialization.cpp \
	SimpleTypeSerializer.cpp \
	StringsAndChecksums.cpp \
	SymbolDumper.cpp \
	SymbolRecordMapping.cpp \
	SymbolSerializer.cpp \
	TypeDumpVisitor.cpp \
	TypeHashing.cpp \
	TypeIndex.cpp \
	TypeIndexDiscovery.cpp \
	TypeRecordMapping.cpp \
	TypeStreamMerger.cpp \
	TypeTableCollection.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
