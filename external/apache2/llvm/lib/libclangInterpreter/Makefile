#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:57 joerg Exp $

LIB=	clangInterpreter

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Interpreter

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/Interpreter

SRCS+=	IncrementalExecutor.cpp \
	IncrementalParser.cpp \
	Interpreter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
