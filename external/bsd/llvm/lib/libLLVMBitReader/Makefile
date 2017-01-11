#	$NetBSD: Makefile,v 1.3 2017/01/11 12:02:28 joerg Exp $

LIB=	LLVMBitReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitcode/Reader

SRCS+=	BitReader.cpp \
	BitcodeReader.cpp \
	BitstreamReader.cpp \
	MetadataLoader.cpp \
	ValueList.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
