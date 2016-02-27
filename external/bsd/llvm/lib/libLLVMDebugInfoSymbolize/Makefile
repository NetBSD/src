#	$NetBSD: Makefile,v 1.1 2016/02/27 21:47:33 joerg Exp $

LIB=	LLVMDebugInfoSymbolize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/Symbolize

SRCS+=	DIPrinter.cpp \
	Symbolize.cpp \
	SymbolizableObjectFile.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
