#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:09 cjep Exp $

LIB=	LLVMObject

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	ArchiveWriter.cpp \
	Binary.cpp \
	COFFImportFile.cpp \
	COFFModuleDefinition.cpp \
	COFFObjectFile.cpp \
	Decompressor.cpp \
	ELF.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	FaultMapParser.cpp \
	IRObjectFile.cpp \
	IRSymtab.cpp \
	MachOObjectFile.cpp \
	MachOUniversal.cpp \
	MachOUniversalWriter.cpp \
	Minidump.cpp \
	ModuleSymbolTable.cpp \
	Object.cpp \
	ObjectFile.cpp \
	RecordStreamer.cpp \
	RelocationResolver.cpp \
	SymbolicFile.cpp \
	SymbolSize.cpp \
	TapiFile.cpp \
	TapiUniversal.cpp \
	WasmObjectFile.cpp \
	WindowsMachineFlag.cpp \
	WindowsResource.cpp \
	XCOFFObjectFile.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
