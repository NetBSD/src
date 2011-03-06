#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:49 bouyer Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	COFFObjectFile.cpp \
	ELFObjectFile.cpp \
	MachOObject.cpp \
	ObjectFile.cpp

.include <bsd.lib.mk>
