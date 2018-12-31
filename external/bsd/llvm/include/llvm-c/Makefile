#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:06 maya Exp $

.include <bsd.init.mk>

.PATH:	${LLVM_SRCDIR}/include/llvm-c

SUBDIR+=	Transforms

INCS=	Analysis.h \
	BitReader.h \
	BitWriter.h \
	Comdat.h \
	Core.h \
	DataTypes.h \
	DebugInfo.h \
	Disassembler.h \
	DisassemblerTypes.h \
	ErrorHandling.h \
	ExecutionEngine.h \
	IRReader.h \
	Initialization.h \
	LinkTimeOptimizer.h \
	Linker.h \
	Object.h \
	OrcBindings.h \
	Support.h \
	Target.h \
	TargetMachine.h \
	Types.h \
	lto.h

INCSDIR=	/usr/include/llvm-c

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
