#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:09:09 tls Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	Binary.cpp \
	COFFObjectFile.cpp \
	COFFYAML.cpp \
	ELF.cpp \
	ELFObjectFile.cpp \
	ELFYAML.cpp \
	Error.cpp \
	IRObjectFile.cpp \
	MachOObjectFile.cpp \
	MachOUniversal.cpp \
	Object.cpp \
	ObjectFile.cpp \
	StringTableBuilder.cpp \
	SymbolicFile.cpp \
	YAML.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
