#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:59 joerg Exp $

LIB=	LLVMMCDisassembler

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCDisassembler

SRCS+=	EDDisassembler.cpp \
	EDOperand.cpp \
	EDInst.cpp \
	EDToken.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
