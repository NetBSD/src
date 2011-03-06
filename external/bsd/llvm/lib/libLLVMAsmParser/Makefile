#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:44 bouyer Exp $

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
