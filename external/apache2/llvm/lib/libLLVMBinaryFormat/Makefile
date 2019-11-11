#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:05 joerg Exp $

LIB=	LLVMBinaryFormat

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/BinaryFormat

SRCS+=	AMDGPUMetadataVerifier.cpp \
	Dwarf.cpp \
	Magic.cpp \
	Minidump.cpp \
	MsgPackDocument.cpp \
	MsgPackDocumentYAML.cpp \
	MsgPackReader.cpp \
	MsgPackWriter.cpp \
	Wasm.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
