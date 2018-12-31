#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:04 maya Exp $

.include <bsd.init.mk>

.PATH:	${LLVM_SRCDIR}/include/llvm/ExecutionEngine

INCS=	ExecutionEngine.h \
	GenericValue.h \
	Interpreter.h \
	JITEventListener.h \
	JITSymbol.h \
	MCJIT.h \
	OProfileWrapper.h \
	ObjectCache.h \
	OrcMCJITReplacement.h \
	RTDyldMemoryManager.h \
	RuntimeDyld.h \
	RuntimeDyldChecker.h \
	SectionMemoryManager.h

INCSDIR=	/usr/include/llvm/ExecutionEngine

.include <bsd.prog.mk>
