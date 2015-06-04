#	$NetBSD: Makefile,v 1.5.2.1 2015/06/04 20:05:44 snj Exp $

LIB=	LLVMRuntimeDyld

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld

SRCS+=	RTDyldMemoryManager.cpp \
	RuntimeDyld.cpp \
	RuntimeDyldChecker.cpp \
	RuntimeDyldELF.cpp \
	RuntimeDyldMachO.cpp

.include <bsd.lib.mk>
