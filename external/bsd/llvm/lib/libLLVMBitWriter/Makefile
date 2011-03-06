#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:45 bouyer Exp $

LIB=	LLVMBitWriter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitcode/Writer

SRCS+=	BitWriter.cpp \
	BitcodeWriter.cpp \
	BitcodeWriterPass.cpp \
	ValueEnumerator.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
