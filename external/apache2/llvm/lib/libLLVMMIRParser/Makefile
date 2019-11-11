#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:09 joerg Exp $

LIB=	LLVMMIRParser

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/MIRParser

SRCS+=	MILexer.cpp \
	MIParser.cpp \
	MIRParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
