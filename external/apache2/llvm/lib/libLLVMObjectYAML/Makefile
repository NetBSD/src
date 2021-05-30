#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:53 joerg Exp $

LIB=	LLVMObjectYAML

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ObjectYAML

SRCS+=	ArchiveEmitter.cpp \
	ArchiveYAML.cpp \
	CodeViewYAMLDebugSections.cpp \
	CodeViewYAMLSymbols.cpp \
	CodeViewYAMLTypeHashing.cpp \
	CodeViewYAMLTypes.cpp \
	COFFEmitter.cpp \
	COFFYAML.cpp \
	DWARFEmitter.cpp \
	DWARFYAML.cpp \
	ELFEmitter.cpp \
	ELFYAML.cpp \
	MachOEmitter.cpp \
	MachOYAML.cpp \
	MinidumpEmitter.cpp \
	MinidumpYAML.cpp \
	ObjectYAML.cpp \
	WasmEmitter.cpp \
	WasmYAML.cpp \
	XCOFFYAML.cpp \
	yaml2obj.cpp \
	YAML.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
