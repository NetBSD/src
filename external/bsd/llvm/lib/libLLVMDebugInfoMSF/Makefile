#	$NetBSD: Makefile,v 1.1 2017/01/11 12:02:29 joerg Exp $

LIB=	LLVMDebugInfoMSF

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/MSF

SRCS+=	MappedBlockStream.cpp \
	MSFBuilder.cpp \
	MSFCommon.cpp \
	MSFError.cpp \
	StreamReader.cpp \
	StreamWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
