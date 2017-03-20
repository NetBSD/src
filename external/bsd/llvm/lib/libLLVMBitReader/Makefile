#	$NetBSD: Makefile,v 1.2.14.1 2017/03/20 06:55:59 pgoyette Exp $

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
