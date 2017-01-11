#	$NetBSD: Makefile,v 1.13 2017/01/11 12:02:30 joerg Exp $

LIB=	LLVMObject

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	ArchiveWriter.cpp \
	Binary.cpp \
	COFFObjectFile.cpp \
	ELF.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	IRObjectFile.cpp \
	MachOObjectFile.cpp \
	MachOUniversal.cpp \
	ModuleSummaryIndexObjectFile.cpp \
	ModuleSymbolTable.cpp \
	Object.cpp \
	ObjectFile.cpp \
	RecordStreamer.cpp \
	SymbolSize.cpp \
	SymbolicFile.cpp \
	WasmObjectFile.cpp \

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
