#	$NetBSD: Makefile,v 1.3.6.2 2014/08/19 23:51:24 tls Exp $

LIB=	LLVMMCDisassembler

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCDisassembler

SRCS+=	Disassembler.cpp \
	MCDisassembler.cpp \
	MCExternalSymbolizer.cpp \
	MCRelocationInfo.cpp \

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
