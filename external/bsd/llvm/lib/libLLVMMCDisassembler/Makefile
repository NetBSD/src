#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:48 bouyer Exp $

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
