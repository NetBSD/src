#	$NetBSD: Makefile,v 1.3 2012/04/04 10:42:00 joerg Exp $

PROG_CXX=	bugpoint
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/bugpoint

SRCS=	BugDriver.cpp \
	CrashDebugger.cpp \
	ExecutionDriver.cpp \
	ExtractFunction.cpp \
	FindBugs.cpp \
	Miscompilation.cpp \
	OptimizerDriver.cpp \
	ToolRunner.cpp \
	bugpoint.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	InstCombine \
	Instrumentation \
	Linker \
	ScalarOpts \
	Target \
	ipo \
	Vectorize \
	TransformsUtils \
	Analysis \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
