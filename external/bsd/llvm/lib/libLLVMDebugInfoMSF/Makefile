#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:55:59 pgoyette Exp $

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
