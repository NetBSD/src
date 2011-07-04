#	$NetBSD: Makefile,v 1.3 2011/07/04 16:10:39 joerg Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Binary.cpp \
	COFFObjectFile.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	MachOObject.cpp \
	MachOObjectFile.cpp \
	Object.cpp \
	ObjectFile.cpp

.include <bsd.lib.mk>
