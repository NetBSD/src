#	$NetBSD: Makefile,v 1.3.2.1 2012/11/20 02:58:02 tls Exp $

LIB=	LLVMTableGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TableGen

SRCS+=	Error.cpp \
	Main.cpp \
	Record.cpp \
	StringMatcher.cpp \
	TableGenBackend.cpp \
	TGLexer.cpp \
	TGParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
