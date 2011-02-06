#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:54 joerg Exp $

LIB=	LLVMBitReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitcode/Reader

SRCS+=	BitReader.cpp \
	BitcodeReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
