#	$NetBSD: Makefile,v 1.11 2019/10/13 07:28:06 mrg Exp $

LIB=	LLVMRuntimeDyld

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld
.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/RuntimeDyld/Targets

SRCS+=	JITSymbol.cpp \
	RTDyldMemoryManager.cpp \
	RuntimeDyld.cpp \
	RuntimeDyldCOFF.cpp \
	RuntimeDyldChecker.cpp \
	RuntimeDyldELF.cpp \
	RuntimeDyldMachO.cpp

SRCS+=	RuntimeDyldELFMips.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
COPTS.RuntimeDyldChecker.cpp+=	${GCC_NO_STRINGOP_OVERFLOW}
.endif
