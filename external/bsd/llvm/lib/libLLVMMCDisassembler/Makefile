#	$NetBSD: Makefile,v 1.2 2011/05/12 14:56:23 joerg Exp $

LIB=	LLVMMCDisassembler

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCDisassembler

SRCS+=	Disassembler.cpp \
	EDDisassembler.cpp \
	EDOperand.cpp \
	EDInst.cpp \
	EDToken.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
