#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:00 joerg Exp $

LIB=	LLVMMCParser

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCParser

SRCS+=	AsmLexer.cpp \
	AsmParser.cpp \
	COFFAsmParser.cpp \
	DarwinAsmParser.cpp \
	ELFAsmParser.cpp \
	MCAsmLexer.cpp \
	MCAsmParser.cpp \
	MCAsmParserExtension.cpp \
	TargetAsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
