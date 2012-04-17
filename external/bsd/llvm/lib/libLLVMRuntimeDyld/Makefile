#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:03:32 yamt Exp $

LIB=	LLVMRuntimeDyld

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld

SRCS+=	RuntimeDyld.cpp \
	RuntimeDyldELF.cpp \
	RuntimeDyldMachO.cpp

.include <bsd.lib.mk>
