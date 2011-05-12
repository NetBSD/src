#	$NetBSD: Makefile,v 1.2 2011/05/12 14:56:23 joerg Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	COFFObjectFile.cpp \
	ELFObjectFile.cpp \
	MachOObject.cpp \
	MachOObjectFile.cpp \
	Object.cpp \
	ObjectFile.cpp

.include <bsd.lib.mk>
