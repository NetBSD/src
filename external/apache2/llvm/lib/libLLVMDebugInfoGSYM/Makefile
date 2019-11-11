#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:05 joerg Exp $

LIB=	LLVMDebugInfoGSYM

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/GSYM

SRCS+=	FileWriter.cpp \
	FunctionInfo.cpp \
	GsymCreator.cpp \
	GsymReader.cpp \
	Header.cpp \
	InlineInfo.cpp \
	LineTable.cpp \
	Range.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
