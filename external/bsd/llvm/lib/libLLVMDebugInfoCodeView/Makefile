#	$NetBSD: Makefile,v 1.2 2017/01/11 12:02:29 joerg Exp $

LIB=	LLVMDebugInfoCodeView

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/CodeView

SRCS+=	CVSymbolVisitor.cpp \
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
	TypeDumper.cpp \
	TypeRecord.cpp \
	TypeRecordMapping.cpp \
	TypeSerializer.cpp \
	TypeStreamMerger.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
