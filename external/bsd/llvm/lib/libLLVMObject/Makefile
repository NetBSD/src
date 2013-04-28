#	$NetBSD: Makefile,v 1.5 2013/04/28 00:41:01 joerg Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	Binary.cpp \
	COFFObjectFile.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	MachOObjectFile.cpp \
	Object.cpp \
	ObjectFile.cpp

.include <bsd.lib.mk>
