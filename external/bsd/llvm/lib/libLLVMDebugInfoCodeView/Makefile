#	$NetBSD: Makefile,v 1.4 2017/08/01 20:07:40 joerg Exp $

LIB=	LLVMDebugInfoCodeView

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/CodeView

SRCS+=	CVSymbolVisitor.cpp \
	CVTypeVisitor.cpp \
	CodeViewError.cpp \
	CodeViewRecordIO.cpp \
	DebugFrameDataSubsection.cpp \
	DebugChecksumsSubsection.cpp \
	DebugCrossExSubsection.cpp \
	DebugSymbolsSubsection.cpp \
	DebugCrossImpSubsection.cpp \
	DebugInlineeLinesSubsection.cpp \
	DebugSubsectionRecord.cpp \
	DebugSubsection.cpp \
	DebugSubsectionVisitor.cpp \
	DebugLinesSubsection.cpp \
	DebugStringTableSubsection.cpp \
	EnumTables.cpp \
	Formatters.cpp \
	Line.cpp \
	RecordSerialization.cpp \
	StringsAndChecksums.cpp \
	SymbolDumper.cpp \
	SymbolRecordMapping.cpp \
	SymbolSerializer.cpp \
	TypeDumpVisitor.cpp \
	TypeIndex.cpp \
	TypeIndexDiscovery.cpp \
	TypeName.cpp \
	LazyRandomTypeCollection.cpp \
	TypeRecordMapping.cpp \
	TypeSerializer.cpp \
	TypeStreamMerger.cpp \
	TypeTableCollection.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
