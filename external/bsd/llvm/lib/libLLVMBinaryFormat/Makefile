#	$NetBSD: Makefile,v 1.1.2.1 2018/07/28 04:37:10 pgoyette Exp $

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
