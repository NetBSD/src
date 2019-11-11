#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:05 joerg Exp $

LIB=	LLVMBitReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitcode/Reader

SRCS+=	BitcodeAnalyzer.cpp \
	BitReader.cpp \
	BitcodeReader.cpp \
	MetadataLoader.cpp \
	ValueList.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
