#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:07 cjep Exp $

LIB=	LLVMDebugInfoGSYM

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/GSYM

SRCS+=	DwarfTransformer.cpp \
	FileWriter.cpp \
	FunctionInfo.cpp \
	GsymCreator.cpp \
	GsymReader.cpp \
	Header.cpp \
	InlineInfo.cpp \
	LineTable.cpp \
	LookupResult.cpp \
	Range.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
