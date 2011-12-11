#	$NetBSD: Makefile,v 1.3 2011/12/11 14:58:50 joerg Exp $

LIB=	LLVMMCDisassembler

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCDisassembler

SRCS+=	Disassembler.cpp \
	EDDisassembler.cpp \
	EDMain.cpp \
	EDOperand.cpp \
	EDInst.cpp \
	EDToken.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
