#	$NetBSD: Makefile,v 1.3 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMObjectYAML

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ObjectYAML

SRCS+=	CodeViewYAMLDebugSections.cpp \
	CodeViewYAMLSymbols.cpp \
	CodeViewYAMLTypeHashing.cpp \
	CodeViewYAMLTypes.cpp \
	COFFYAML.cpp \
	DWARFEmitter.cpp \
	DWARFVisitor.cpp \
	DWARFYAML.cpp \
	ELFYAML.cpp \
	MachOYAML.cpp \
	ObjectYAML.cpp \
	WasmYAML.cpp \
	YAML.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
