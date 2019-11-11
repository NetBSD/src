#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:05 joerg Exp $

LIB=	LLVMBitstreamReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Bitstream/Reader

SRCS+=	BitstreamReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
