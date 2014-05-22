#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:14:41 yamt Exp $

LIB=	LLVMBitReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitcode/Reader

SRCS+=	BitReader.cpp \
	BitcodeReader.cpp \
	BitstreamReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
