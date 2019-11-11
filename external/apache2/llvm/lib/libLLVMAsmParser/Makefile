#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:04 joerg Exp $

LIB=	LLVMAsmParser

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/AsmParser

SRCS+=	LLLexer.cpp \
	LLParser.cpp \
	Parser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
