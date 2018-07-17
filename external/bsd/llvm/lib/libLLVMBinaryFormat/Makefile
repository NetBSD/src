#	$NetBSD: Makefile,v 1.2 2018/07/17 18:55:27 joerg Exp $

LIB=	LLVMBinaryFormat

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/BinaryFormat

SRCS+=	Dwarf.cpp \
	Magic.cpp \
	Wasm.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
