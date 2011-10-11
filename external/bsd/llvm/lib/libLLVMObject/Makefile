#	$NetBSD: Makefile,v 1.4 2011/10/11 13:53:59 joerg Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	Binary.cpp \
	COFFObjectFile.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	MachOObject.cpp \
	MachOObjectFile.cpp \
	Object.cpp \
	ObjectFile.cpp

.include <bsd.lib.mk>
