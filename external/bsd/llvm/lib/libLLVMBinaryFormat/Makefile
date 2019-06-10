#	$NetBSD: Makefile,v 1.1.4.1 2019/06/10 21:50:51 christos Exp $

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
