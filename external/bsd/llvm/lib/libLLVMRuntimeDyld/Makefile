#	$NetBSD: Makefile,v 1.2.2.2 2012/05/23 10:07:27 yamt Exp $

LIB=	LLVMRuntimeDyld

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld

SRCS+=	GDBRegistrar.cpp \
	RuntimeDyld.cpp \
	RuntimeDyldELF.cpp \
	RuntimeDyldMachO.cpp

.include <bsd.lib.mk>
