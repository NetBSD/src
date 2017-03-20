#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:00 pgoyette Exp $

LIB=	LLVMObjectYAML

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ObjectYAML

SRCS+=	COFFYAML.cpp \
	DWARFYAML.cpp \
	ELFYAML.cpp \
	MachOYAML.cpp \
	ObjectYAML.cpp \
	YAML.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
