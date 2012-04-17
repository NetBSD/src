#	$NetBSD: Makefile,v 1.1.2.1 2012/04/17 00:03:32 yamt Exp $

LIB=	LLVMTableGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TableGen

SRCS+=	Error.cpp \
	Main.cpp \
	Record.cpp \
	TableGenAction.cpp \
	TableGenBackend.cpp \
	TGLexer.cpp \
	TGParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
