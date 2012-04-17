#	$NetBSD: Makefile,v 1.2.4.1 2012/04/17 00:03:31 yamt Exp $

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
