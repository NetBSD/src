#	$NetBSD: Makefile,v 1.2 2011/07/13 23:48:30 joerg Exp $

LIB=	LLVMRuntimeDyld

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld

SRCS+=	RuntimeDyld.cpp \
	RuntimeDyldMachO.cpp

.include <bsd.lib.mk>
