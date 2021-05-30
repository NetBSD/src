#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:53 joerg Exp $

LIB=	LLVMMCParser

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCParser

SRCS+=	AsmLexer.cpp \
	AsmParser.cpp \
	COFFAsmParser.cpp \
	COFFMasmParser.cpp \
	DarwinAsmParser.cpp \
	ELFAsmParser.cpp \
	MasmParser.cpp \
	MCAsmLexer.cpp \
	MCAsmParser.cpp \
	MCAsmParserExtension.cpp \
	MCTargetAsmParser.cpp \
	WasmAsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
