#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 16:13:21 yamt Exp $

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
	ObjCARC \
	IRReader \
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
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
