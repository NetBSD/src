#	$NetBSD: Makefile,v 1.2.2.1 2018/07/28 04:37:11 pgoyette Exp $

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
